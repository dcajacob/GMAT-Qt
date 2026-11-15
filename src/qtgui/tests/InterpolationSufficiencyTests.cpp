#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "EphemerisFile.hpp"
#include "LagrangeInterpolator.hpp"
#include "SpiceOrbitKernelWriter.hpp"
#include "ICRFFile.hpp"
#include "EphemWriterSPK.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextDocument>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
void require(bool ok,const QString &message) {if(!ok)throw std::runtime_error(message.toStdString());}
void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Cannot retain interpolation evidence");}
QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Interpolation output missing: "+path);return f.readAll();}
QString num(double v) {return QString::number(v,'g',17);}
using State=std::array<double,6>;
constexpr double mu=398600.4415;
State circle(double t) {const double n=std::sqrt(mu/(7000.*7000.*7000.)),a=n*t,v=std::sqrt(mu/7000.);return {7000*std::cos(a),7000*std::sin(a),0,-v*std::sin(a),v*std::cos(a),0};}
// Closed-form polynomial and derivative, independent of either interpolator.
double polynomial(double x,int degree,int axis,bool derivative=false) {
   long double value=derivative?0:7000.+axis;
   for(int j=derivative?1:0;j<=degree;++j) {
      const long double c=(axis+1)*.02L*(j+1)*((j+axis)%2?-1:1);
      value+=c*(derivative?j:1)*std::pow(static_cast<long double>(x),derivative?j-1:j);
   }
   return double(value);
}
QJsonArray lagrange() {
   QJsonArray proof;
   for(int order=1;order<=10;++order)for(int direction:{1,-1}) {
      LagrangeInterpolator interpolation("Polynomial",3,order);interpolation.SetForceInterpolation(true);
      auto point=[&](int i) {const double x=direction*(-1.+2.*i/order);double y[3];for(int a=0;a<3;++a)y[a]=polynomial(x,order,a);require(interpolation.AddPoint(x,y),"Lagrange rejected a distinct sample");return x;};
      for(int i=0;i<order;++i)point(i);
      require(interpolation.GetPointCount()==order&&interpolation.IsInterpolationFeasible(0)==-1,"Lagrange accepted fewer independent samples than its degree requires");
      const auto last=point(order-1);require(interpolation.GetPointCount()==order,"A repeated epoch falsely supplied another constraint");
      double replacement[3];for(int a=0;a<3;++a)replacement[a]=polynomial(last,order,a)+1;
      interpolation.AddPoint(last,replacement);point(order-1);require(interpolation.GetPointCount()==order,"Duplicate replacement changed sample count");point(order);
      double largest=0;
      for(double x:{-.93,-.51,-.17,.13,.47,.89}) {double y[3];require(interpolation.Interpolate(x,y),"Sufficient Lagrange polynomial refused interpolation");for(int a=0;a<3;++a){largest=std::max(largest,std::abs(y[a]-polynomial(x,order,a)));require(std::isfinite(y[a])&&std::abs(y[a]-polynomial(x,order,a))<=1e-9,"Lagrange differs from independent polynomial");}}
      interpolation.Clear();require(interpolation.GetPointCount()==0,"Clear retained interpolation samples");
      double a[3]={1,2,3},b[3]={3,4,5},mid[3];interpolation.AddPoint(0,a);interpolation.AddPoint(direction,b);
      if(order>1) {require(interpolation.IsInterpolationFeasible(direction*.5)==-1&&interpolation.AdjustOrderToNumPoints(),"Compatibility order reduction unavailable");require(interpolation.Interpolate(direction*.5,mid),"Reduced linear interpolation failed");for(int j=0;j<3;++j)require(std::abs(mid[j]-(j+2))<=1e-12,"Reduced interpolation differs from linear oracle");interpolation.Clear();interpolation.AddPoint(0,a);interpolation.AddPoint(direction,b);require(interpolation.IsInterpolationFeasible(direction*.5)==-1,"Clear retained a temporary reduced order");}
      proof.append(QJsonObject{{"order",order},{"direction",direction},{"minimum_independent_samples",order+1},{"duplicate_replacement",true},{"base_order_restored",true},{"max_error",largest}});
   }
   return proof;
}
struct Rig {
   MainWindow window;QPlainTextEdit *source,*messages;QString directory;int builds=0;
   Rig(const QString &startup,const QString &output):directory(output) {require(window.initialize(startup),"Interpolation runtime initialization failed");window.show();acceptanceView(window);source=window.findChild<QPlainTextEdit*>("scriptEditor");messages=window.findChild<QPlainTextEdit*>("messageWindow");require(source&&messages,"Interpolation workspace missing");}
   ~Rig(){source->document()->setModified(false);}
   void build(const QString &script,const QString &name) {++builds;source->setPlainText(script);messages->clear();write(directory+"/"+name+".script",script.toUtf8());const bool ok=window.buildScript();write(directory+"/"+name+"-build.txt",messages->toPlainText().toUtf8());require(ok,"Interpolation Build failed: "+messages->toPlainText());}
   MainWindow::RunResult run(const QString &name) {const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();const auto status=window.runMission();QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();write(directory+"/"+name+"-messages.txt",messages->toPlainText().toUtf8());require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Interpolation run changed source/history");return status;}
};
QJsonArray spk(const QString &startup,const QString &directory) {
   Rig rig(startup,directory);QJsonArray proof;erract_c("SET",0,const_cast<char*>("RETURN"));auto *frames=ICRFFile::Instance();frames->Initialize();
   for(int degree:{1,3,5,7,9}) {
      const int minimum=std::max(2,(degree+1)/2);const auto path=directory+"/degree-"+QString::number(degree)+".bsp";const int target=-199900-degree;
      SpiceOrbitKernelWriter writer("Polynomial","Earth",target,399,path.toStdString(),degree);require(writer.GetMinNumberOfStates()==minimum,"SPK minimum differs from independent Hermite constraint count");
      std::vector<std::unique_ptr<A1Mjd>> epochs;std::vector<std::unique_ptr<Rvector6>> states;std::vector<GmatTime> precise;std::vector<bool> flags;EpochArray times;StateArray values;std::vector<double> ets;
      // The writer accepts native MJ2000Eq/FK5 input for J2000 output. Normalize
      // the analytic ICRF fixture to that input basis; this is not an independent
      // qualification of the FK5/ICRF frame model itself.
      for(int i=0;i<minimum;++i) {GmatTime t(21545.);t.AddSeconds(100.*i/(minimum-1));precise.push_back(t);flags.push_back(true);ets.push_back(writer.A1ToSpiceTime(t));epochs.emplace_back(new A1Mjd(t.GetMjd()));times.push_back(epochs.back().get());const double x=(ets.back()-ets.front())/100.;State state,fk5;for(int a=0;a<3;++a){state[a]=polynomial(x,degree,a);state[a+3]=polynomial(x,degree,a,true)/100.;}frames->RotateFromICRFtoFK5(state.data(),fk5.data(),t.GetMjd());states.emplace_back(new Rvector6(fk5[0],fk5[1],fk5[2],fk5[3],fk5[4],fk5[5]));values.push_back(states.back().get());}
      auto tooFewTimes=times;auto tooFewValues=values;tooFewTimes.pop_back();tooFewValues.pop_back();bool rejected=false;
      try {writer.WriteSegment(*epochs.front(),*epochs.back(),tooFewValues,tooFewTimes);}catch(BaseException&){rejected=true;}
      require(rejected&&!QFileInfo::exists(path),"Insufficient SPK records opened output or were accepted");
      auto bad=precise;bad[1]=bad[0];rejected=false;try{writer.WriteSegment(*epochs.front(),*epochs.back(),values,times,bad,flags);}catch(BaseException&){rejected=true;}require(rejected&&!QFileInfo::exists(path),"Duplicate SPK epochs were accepted or opened output");
      bad=precise;std::reverse(bad.begin(),bad.end());rejected=false;try{writer.WriteSegment(*epochs.front(),*epochs.back(),values,times,bad,flags);}catch(BaseException&){rejected=true;}require(rejected&&!QFileInfo::exists(path),"Backward SPK epochs were accepted or opened output");
      writer.WriteSegment(*epochs.front(),*epochs.back(),values,times,precise,flags);writer.FinalizeKernel();require(QFileInfo(path).size()>0,"Exact-minimum SPK output missing");
      SpiceInt handle;spklef_c(path.toUtf8().constData(),&handle);require(!failed_c(),"Cannot read generated type-13 SPK");double maxPosition=0,maxVelocity=0;
      for(int i=0;i<=16;++i) {const double et=ets.front()+(ets.back()-ets.front())*i/16.;double actual[6],light;spkgeo_c(target,et,"J2000",399,actual,&light);require(!failed_c(),"CSPICE rejected sufficient Hermite records");const double x=(et-ets.front())/100.;for(int a=0;a<3;++a){maxPosition=std::max(maxPosition,std::abs(actual[a]-polynomial(x,degree,a)));maxVelocity=std::max(maxVelocity,std::abs(actual[a+3]-polynomial(x,degree,a,true)/100.));}}
      spkuef_c(handle);require(maxPosition<=2e-8&&maxVelocity<=2e-10,"Encoded Hermite curve differs from independent polynomial/derivative");
      proof.append(QJsonObject{{"degree",degree},{"minimum_states",minimum},{"queries",17},{"preflight_failures",3},{"max_position_error_km",maxPosition},{"max_velocity_error_km_s",maxVelocity},{"native_input_frame_normalization",true},{"physical_fk5_frame_qualified",false}});
   }
   return proof;
}
QString config(const QString &path,const QString &format,int order,const QString &action) {
   return "Earth.Mu = "+num(mu)+";\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+num(std::sqrt(mu/7000.))+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nP.Accuracy = 1e-12;\nCreate EphemerisFile Export;\nExport.Spacecraft = Sat;\nExport.Filename = '"+path+"';\nExport.FileFormat = "+format+";\nExport.CoordinateSystem = EarthMJ2000Eq;\nExport.StepSize = 5;\nExport.InterpolationOrder = "+QString::number(order)+";\nExport.InsufficientDataAction = "+action+";\n";
}
QJsonArray backgroundSpk(const QString &startup,const QString &directory) {
   Rig rig(startup,directory);rig.build("Create Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.NAIFId = -199970;\nBeginMissionSequence;\n","background-setup");
   auto *sat=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject("Sat"));auto *frame=Moderator::Instance()->GetInternalCoordinateSystem();require(sat&&frame,"Native SPK context missing");QJsonArray proof;
   for(int degree:{1,3,5,7,9})for(bool background:{false,true}) {
      const int minimum=std::max(2,(degree+1)/2);const auto path=directory+"/"+(background?"scratch-":"user-")+QString::number(degree)+".bsp";
      EphemWriterSPK writer("Sufficiency","SPK");EphemerisWriter &api=writer;api.SetFileName(path.toStdString(),path.toStdString(),"");api.SetSpacecraft(sat);api.SetDataCoordSystem(frame);api.SetOutCoordSystem(frame);api.SetInitialData("InitialSpacecraftEpoch","FinalSpacecraftEpoch","IntegratorSteps",-999.999,false,"Hermite",degree);api.SetInitialTime(-999.999,-999.999);api.SetBackgroundGeneration(background);require(api.Initialize(),"Native SPK writer initialization failed");api.CreateEphemerisFile(false,"Cartesian","LittleEndian","None");
      auto sample=[&](int i) {GmatTime epoch(21545.);epoch.AddSeconds(i*10.);auto state=circle(i*10.);double covariance[21]={},quaternion[4]={1,0,0,0},acceleration[3]={};writer.SetPublishedEpoch(&epoch);api.SetOrbitData(epoch.GetMjd(),state.data(),covariance,quaternion,acceleration);api.SetEpochAndDirection(i?GmatTime(21545.).GetMjd()*86400+(i-1)*10:-999.999,epoch.GetMjd()*86400,1,1);api.HandleOrbitData();};
      for(int i=0;i<minimum-1;++i)sample(i);api.SetRunFlags(false,false,false);bool rejected=false;try{api.FinishUpWriting();}catch(BaseException&){rejected=true;}
      require(rejected==!background&&!QFileInfo::exists(path),"User/scratch SPK insufficiency policy differs");
      bool unwritten=false;if(background) {unwritten=api.InsufficientDataPoints();require(unwritten==(minimum-1>1),"Scratch SPK pending-data flag differs");sample(minimum-1);api.SetRunFlags(true,true,false);api.FinishUpWriting();require(QFileInfo(path).size()>0&&!api.InsufficientDataPoints(),"Scratch SPK did not retain and complete its short sample buffer");}
      proof.append(QJsonObject{{"degree",degree},{"minimum_states",minimum},{"background",background},{"short_input_rejected",rejected},{"unwritten_data_flag",unwritten},{"scratch_completed_after_more_data",background}});
   }
   return proof;
}
double newton(double t,int degree,int component) {
   std::vector<long double> differences;for(int i=0;i<=degree;++i)differences.push_back(circle(i*10.)[component]);
   for(int j=1;j<=degree;++j)for(int i=degree;i>=j;--i)differences[i]=(differences[i]-differences[i-1])/(10.L*j);
   long double value=differences[degree];for(int i=degree-1;i>=0;--i)value=value*(t-10.L*i)+differences[i];return double(value);
}
void checkOemHeader(const QString &text) {
   require(text.startsWith("CCSDS_OEM_VERS = 1.0\n")&&text.contains("CREATION_DATE  = ")&&text.contains("ORIGINATOR     = GMAT USER"),"OEM header is missing or malformed");
}
int checkOem(const QString &path,int inputDegree) {
   const auto text=QString::fromUtf8(read(path));checkOemHeader(text);double first=0;int rows=0;
   for(const auto &line:text.split('\n')) {const auto words=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(words.size()!=7||!words[0].contains('T'))continue;const auto stamp=words[0];const auto date=QDateTime::fromString(stamp.left(19),"yyyy-MM-ddTHH:mm:ss");require(date.isValid(),"OEM state timestamp invalid");const double seconds=date.toMSecsSinceEpoch()/1000.+stamp.mid(19).toDouble();if(!rows)first=seconds;const double t=seconds-first;require(std::abs(t-5.*rows)<5e-5,"OEM output grid differs");for(int c=0;c<6;++c){bool ok=false;const double actual=words[c+1].toDouble(&ok);require(ok&&std::isfinite(actual)&&std::abs(actual-newton(t,inputDegree,c))<=(c<3?2e-5:2e-8),"OEM interpolation differs from independent Newton polynomial");}++rows;}
   require(rows==inputDegree*2+1,"OEM omitted or duplicated requested epochs");return rows;
}
void checkSingleOem(const QString &path) {
   const auto text=QString::fromUtf8(read(path));checkOemHeader(text);const QString stamp="2000-01-01T12:00:05.000";int rows=0;
   for(const auto &key:QStringList{"START_TIME","STOP_TIME","USEABLE_START_TIME","USEABLE_STOP_TIME"})
      require(text.contains(QRegularExpression("(?m)^"+key+"\\s*=\\s*"+QRegularExpression::escape(stamp))),"Single-record OEM metadata epoch differs: "+key);
   require(text.contains(QRegularExpression("INTERPOLATION_DEGREE\\s*=\\s*0(?:\\s|$)")),"Single-record OEM advertises unsupported interpolation degree");
   for(const auto &line:text.split('\n')) {const auto words=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(words.size()!=7||!words[0].contains('T'))continue;require(words[0].startsWith(stamp),"Single-record OEM exported the wrong epoch");for(int c=0;c<6;++c){bool ok=false;const double actual=words[c+1].toDouble(&ok);require(ok&&std::isfinite(actual)&&std::abs(actual-newton(5,1,c))<=(c<3?2e-5:2e-8),"Single-record OEM differs from independently interpolated state");}++rows;}
   require(rows==1,"Single-record OEM emitted extra or missing records");
}
void checkSingleStk(const QString &path) {
   const auto text=QString::fromUtf8(read(path));require(text.contains("ScenarioEpoch           01 Jan 2000 12:00:05.000")&&text.contains("DistanceUnit            Kilometers"),"Single-record STK metadata differs");int rows=0;
   for(const auto &line:text.split('\n')) {const auto words=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(words.size()!=7)continue;bool numeric=false;const double t=words[0].toDouble(&numeric);if(!numeric)continue;require(std::abs(t)<=1e-8,"Single-record STK relative epoch differs");for(int c=0;c<6;++c){bool ok=false;const double actual=words[c+1].toDouble(&ok);require(ok&&std::isfinite(actual)&&std::abs(actual-newton(5,1,c))<=(c<3?2e-5:2e-8),"Single-record STK differs from independently interpolated state");}++rows;}
   require(rows==1&&text.contains(QRegularExpression("NumberOfEphemerisPoints\\s+1(?:\\s|$)")),"Single-record STK emitted extra or missing records");
}
void checkWindow(const QString &path,const QString &format) {
   const auto text=QString::fromUtf8(read(path));int rows=0;const bool oem=format=="CCSDS-OEM";if(oem)checkOemHeader(text);
   require(text.contains(oem?"2000-01-01T12:00:03.000":"ScenarioEpoch           01 Jan 2000 12:00:03.000"),"Explicit window start metadata differs");
   for(const auto &line:text.split('\n')) {const auto words=line.trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(words.size()!=7)continue;bool numeric=false;const double relative=words[0].toDouble(&numeric);if(oem?!words[0].contains('T'):!numeric)continue;require(rows<2,"Explicit window duplicated records");const double t=rows?8:3;
      require(oem?words[0].startsWith(rows?"2000-01-01T12:00:08.000":"2000-01-01T12:00:03.000"):std::abs(relative-5*rows)<=5e-5,"Explicit window epoch differs");
      for(int c=0;c<6;++c){bool ok=false;const double actual=words[c+1].toDouble(&ok);require(ok&&std::isfinite(actual)&&std::abs(actual-newton(t,1,c))<=(c<3?2e-5:2e-8),"Explicit window differs from independently interpolated state");}++rows;}
   require(rows==2,"Explicit window omitted requested epochs");
}
QJsonArray policy(const QString &startup,const QString &directory,bool boundaries) {
   Rig rig(startup,directory);QJsonArray proof;
   for(const auto &format:QStringList{"CCSDS-OEM","STK-TimePosVel","Code-500"})for(int order=1;order<=10;++order) {
      const auto scenarios=boundaries?QStringList{"Assignment","Maneuver","Toggle","Explicit","Window"}:QStringList{"Short","Exact"};
      for(const auto &scenario:scenarios)for(const auto &action:QStringList{"Error","ReduceOrder"}) {
         if(scenario=="Exact"&&action=="ReduceOrder")continue;
         if((scenario=="Explicit"||scenario=="Window")&&format=="Code-500")continue;
         const auto name=format+"-"+QString::number(order)+"-"+scenario+"-"+action,path=directory+"/"+name+".eph";QString script=config(path,format,order,action);
         if(scenario=="Maneuver")script+="Create ImpulsiveBurn Kick;\nKick.Element1 = 0.001;\n";
         if(scenario=="Explicit"||scenario=="Window") {const auto start=scenario=="Window"?"03":"05",stop=scenario=="Window"?"08":"05";script+="Sat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nExport.EpochFormat = UTCGregorian;\nExport.InitialEpoch = '01 Jan 2000 12:00:"+QString(start)+".000';\nExport.FinalEpoch = '01 Jan 2000 12:00:"+QString(stop)+".000';\n";}
         script+="BeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = "+QString::number(scenario=="Exact"?10*order:10)+"};\n";
         if(scenario=="Assignment")script+="Sat.X = Sat.X + 1;\nPropagate P(Sat) {Sat.ElapsedSecs = 100};\n";
         if(scenario=="Maneuver")script+="Maneuver Kick(Sat);\nPropagate P(Sat) {Sat.ElapsedSecs = 100};\n";
         if(scenario=="Toggle")script+="Toggle Export Off;\nPropagate P(Sat) {Sat.ElapsedSecs = 10};\nToggle Export On;\nPropagate P(Sat) {Sat.ElapsedSecs = 100};\n";
         // Code-500 is one continuous interpolation span, including across
         // maneuvers/assignments/toggles (Resource_EphemerisFile guide). The
         // later samples satisfy its order; no short event segment is closed.
         const bool closesShortSegment=!boundaries||format!="Code-500";
         rig.build(script,name);auto *configured=Moderator::Instance()->GetConfiguredObject("Export");require(configured&&configured->GetStringParameter("InsufficientDataAction")==action.toStdString(),"Interpolation action was not configured");const bool singleOemRecord=scenario=="Explicit"&&format=="CCSDS-OEM"&&order==1;const bool insufficient=action=="Error"&&scenario!="Exact"&&(order>1||singleOemRecord)&&closesShortSegment;const int expectedCount=singleOemRecord?1:2;const auto status=rig.run(name);
         if(insufficient) {const auto message=rig.messages->toPlainText();require(status==MainWindow::RunResult::Failed&&message.contains(path)&&message.contains("Insufficient data")&&message.contains("order "+QString::number(order))&&message.contains("requires at least "+QString::number(order+1))&&message.contains("received "+QString::number(expectedCount))&&message.contains(singleOemRecord?"exported OEM records":"interpolation input samples"),"Strict interpolation did not report the requested order/sample failure: "+name+"\n"+message);}
         else {require(status==MainWindow::RunResult::Completed,"Compatible/sufficient interpolation failed: "+name+"\n"+rig.messages->toPlainText());require(QFileInfo(path).size()>0,"Successful interpolation produced no file");if(format=="CCSDS-OEM"&&!boundaries)checkOem(path,scenario=="Exact"?order:1);if(format=="CCSDS-OEM"&&scenario=="Explicit")checkSingleOem(path);if(format=="STK-TimePosVel"&&scenario=="Explicit")checkSingleStk(path);if(scenario=="Window")checkWindow(path,format);}
         proof.append(QJsonObject{{"format",format},{"order",order},{"scenario",scenario},{"action",action},{"strict_failure_expected",insufficient},{"run_failed",status==MainWindow::RunResult::Failed},{"continuous_code500_span",boundaries&&format=="Code-500"},{"expected_insufficient_sample_count",insufficient?expectedCount:0},{"single_record_oem_guard",singleOemRecord}});write(directory+"/observations.json",QJsonDocument(proof).toJson());
      }
   }
   return proof;
}
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("InterpolationSufficiency");
   try {require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and interpolation case");TestSettings settings;QTemporaryDir scratch;require(scratch.isValid(),"Interpolation temporary directory failed");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto selected=QString::fromLocal8Bit(argv[2]),retained=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"),directory=retained.isEmpty()?scratch.path():retained;QJsonArray proof;
      if(selected=="Lagrange")proof=lagrange();else if(selected=="Spk")proof=spk(startup,directory);else if(selected=="SpkBackground")proof=backgroundSpk(startup,directory);else if(selected=="Policy")proof=policy(startup,directory,false);else if(selected=="Boundaries")proof=policy(startup,directory,true);else throw std::runtime_error("Unknown interpolation case");
      write(directory+"/verification.json",QJsonDocument(QJsonObject{{"case",selected},{"status","passed"},{"observations",proof},{"new_requirement_credit",0},{"whole_row_qualified",false}}).toJson());std::cout<<selected.toStdString()<<" passed: "<<proof.size()<<" observations\n";return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
