#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include "QtMessageReceiver.hpp"
#include "MessageInterface.hpp"
#include "AccelerationFrameOracle.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <sstream>
#include <stdexcept>
#include <iostream>
using namespace accelerationOracle;
static void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
static QByteArray read(const QString&path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Acceleration handoff report missing");return f.readAll();}
static void write(const QString&path,const QByteArray&bytes){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Acceleration handoff evidence write failed");}
// Independent origin oracle: five-point position second difference over60s,
// rather than the production velocity first difference over1s. Native lunar
// ephemerides are shared inputs, not independently qualified ephemeris physics.
static V lunarAcceleration(CelestialBody *moon,const GmatTime&epoch)
{
   auto position=[&](double sec){GmatTime t=epoch;t.AddSeconds(sec);return moon->GetMJ2000Position(t);};
   const auto m2=position(-120),m1=position(-60),zero=position(0),p1=position(60),p2=position(120);position(0);V result{};
   for(int i=0;i<3;++i)result[i]=(-p2[i]+16*p1[i]-30*zero[i]+16*m1[i]-m2[i])/(12*60*60);return result;
}
int main(int argc,char**argv)
{
   QCoreApplication app(argc,argv);
   try {
      QtMessageReceiver messages;messages.SuspendLogging(true);
      messages.SetMessageCallback(&app,[](const QString &text){std::cerr<<text.toStdString();});
      MessageInterface::SetMessageReceiver(&messages);
      struct DetachMessages {~DetachMessages(){MessageInterface::SetMessageReceiver(nullptr);}} detach;
      require(argc==2,"Supply startup for acceleration handoffs");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *m=Moderator::Instance();require(m->Initialize(startup.toStdString(),false),"Acceleration handoff runtime unavailable");QTemporaryDir files;require(files.isValid(),"Acceleration handoff directory unavailable");const auto report=files.filePath("handoff.csv");
      QString source="Create Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 130;\nSat.Z = -200;\nSat.VX = -0.3;\nSat.VY = 7.8;\nSat.VZ = 0.8;\nCreate CoordinateSystem Frame;\nFrame.Origin = Earth;\nFrame.Axes = ObjectReferenced;\nFrame.Primary = Earth;\nFrame.Secondary = Sat;\nFrame.XAxis = V;\nFrame.YAxis = N;\nSat.Attitude = CoordinateSystemFixed;\nSat.AttitudeCoordinateSystem = Frame;\nSat.AttitudeRateDisplayStateType = AngularVelocity;\nCreate ForceModel EarthForces MoonForces;\nEarthForces.CentralBody = Earth;\nEarthForces.PrimaryBodies = {};\nEarthForces.PointMasses = {Earth};\nEarthForces.Drag = None;\nEarthForces.SRP = Off;\nMoonForces.CentralBody = Luna;\nMoonForces.PrimaryBodies = {};\nMoonForces.PointMasses = {Luna};\nMoonForces.Drag = None;\nMoonForces.SRP = Off;\nCreate Propagator First Second;\nFirst.FM = EarthForces;\nSecond.FM = MoonForces;\n";
      for(const auto &name:QStringList{"First","Second"}) source+=name+".Type = PrinceDormand78;\n"+name+".MinStep = 0;\n"+name+".MaxStep = 1;\n"+name+".Accuracy = 1e-12;\n";
      source+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\n";
      const QString output="Report Values Sat.A1ModJulian Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ Sat.AngularVelocityX Sat.AngularVelocityY Sat.AngularVelocityZ;\n";
      source+=output+"Propagate First(Sat) {Sat.ElapsedSecs = 1};\n"+output+"Propagate Second(Sat) {Sat.ElapsedSecs = 1};\n"+output+"Propagate First(Sat) {Sat.ElapsedSecs = 1};\n"+output;
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty())write(evidence+"/handoff.script",source.toUtf8());
      std::istringstream script(source.toStdString());const bool interpreted=m->InterpretScript(&script,true);app.processEvents();require(interpreted,"Acceleration handoff script failed");const auto status=m->RunMission();app.processEvents();
      if(!evidence.isEmpty()&&QFileInfo::exists(report))write(evidence+"/partial-handoff.csv",read(report));
      require(status==1,"Acceleration handoff mission failed");const auto bytes=read(report);if(!evidence.isEmpty())write(evidence+"/handoff.csv",bytes);
      const auto rows=QString::fromUtf8(bytes).trimmed().split('\n');require(rows.size()==4,"Acceleration handoff row count differs");
      auto *earth=m->GetSolarSystemInUse()->GetBody("Earth"),*moon=m->GetSolarSystemInUse()->GetBody("Luna");require(earth&&moon,"Acceleration handoff bodies missing");const double earthMu=earth->GetRealParameter(earth->GetParameterID("Mu")),moonMu=moon->GetRealParameter(moon->GetParameterID("Mu"));QJsonArray observations;
      for(int row=0;row<4;++row) {
         const auto columns=rows[row].split(',');require(columns.size()==10,"Acceleration handoff columns differ");std::array<double,10> values{};
         for(int i=0;i<10;++i){bool ok=false;values[i]=columns[i].toDouble(&ok);require(ok&&std::isfinite(values[i]),"Acceleration handoff output nonfinite");}
         const GmatTime epoch(values[0]);V r{{values[1],values[2],values[3]}},v{{values[4],values[5],values[6]}},a{};double norm=0;
         if(row==2) {
            const auto lunar=moon->GetMJ2000Position(epoch);V relative{};for(int i=0;i<3;++i){relative[i]=r[i]-lunar[i];norm+=relative[i]*relative[i];}norm=std::sqrt(norm);const auto origin=lunarAcceleration(moon,epoch);
            for(int i=0;i<3;++i)a[i]=-moonMu*relative[i]/(norm*norm*norm)+origin[i];
         } else {for(auto x:r)norm+=x*x;norm=std::sqrt(norm);for(int i=0;i<3;++i)a[i]=-earthMu*r[i]/(norm*norm*norm);}
         const auto expected=rateDegrees(r,v,a);QJsonArray actualJson,expectedJson;
         for(int i=0;i<3;++i){actualJson.append(values[7+i]);expectedJson.append(expected[i]);require(std::abs(values[7+i]-expected[i])<5e-9,"Initial/command-switch/origin/repeated-command angular rate differs from independent force and geometric derivative");}
         observations.append(QJsonObject{{"row",row},{"force_origin",row==2?"Luna":"Earth"},{"epoch",values[0]},{"actual_degrees_s",actualJson},{"expected_degrees_s",expectedJson},{"bound_degrees_s",5e-9}});
      }
      if(!evidence.isEmpty())write(evidence+"/handoff-values.json",QJsonDocument(observations).toJson());
      std::cout<<"PASS acceleration handoffs: initial report uses first initialized model; Earth -> lunar-origin -> repeated Earth command, four accepted states with independently constructed VNB rates at5e-9deg/s. Shared native ephemeris inputs, separate position-difference origin oracle.\n";
      m->Finalize();return 0;
   }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
