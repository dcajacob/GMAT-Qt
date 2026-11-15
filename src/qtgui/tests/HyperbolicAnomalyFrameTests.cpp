#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "Parameter.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
constexpr double pi=3.14159265358979323846,mu=398600.4415;
void require(bool value,const char *message) {if (!value) throw std::runtime_error(message);}
void write(const QString &path,const QByteArray &bytes) {
   QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"HA frame artifact write failed");
}
QJsonArray array(const Rvector6 &state) {QJsonArray a;for(int i=0;i<6;++i)a.append(state[i]);return a;}
// Independent perifocal geometry. No GMAT conversion helper supplies truth.
Rvector6 conic(double anomaly,bool hyperbolic=true) {
   const double a=hyperbolic?-12000:10000,e=hyperbolic?1.4:.2;
   const double h=anomaly*pi/180;
   const double ta=hyperbolic?2*std::atan(std::sqrt((e+1)/(e-1))*std::tanh(h/2)):h;
   const double inc=30*pi/180,node=40*pi/180,peri=50*pi/180,p=a*(1-e*e),r=p/(1+e*std::cos(ta));
   const double u[]={std::cos(node)*std::cos(peri)-std::sin(node)*std::sin(peri)*std::cos(inc),
      std::sin(node)*std::cos(peri)+std::cos(node)*std::sin(peri)*std::cos(inc),std::sin(peri)*std::sin(inc)};
   const double v[]={-std::cos(node)*std::sin(peri)-std::sin(node)*std::cos(peri)*std::cos(inc),
      -std::sin(node)*std::sin(peri)+std::cos(node)*std::cos(peri)*std::cos(inc),std::cos(peri)*std::sin(inc)};
   Rvector6 result;for(int i=0;i<3;++i) {result[i]=r*(std::cos(ta)*u[i]+std::sin(ta)*v[i]);
      result[i+3]=std::sqrt(mu/p)*(-std::sin(ta)*u[i]+(e+std::cos(ta))*v[i]);}return result;
}
class PrescribedAxes:public AxisSystem {
public:
   double rate;
   int preciseCalls=0;
   explicit PrescribedAxes(double omega):AxisSystem("HAContractAxes"),rate(omega) {baseSystem="FK5";}
   GmatBase *Clone() const override {return new PrescribedAxes(*this);}
   void rotation(const GmatTime &epoch) {
      const double theta=.37+rate*(epoch-GmatTime(21545.125)).GetTimeInSec(),c=std::cos(theta),s=std::sin(theta);
      rotMatrix.Set(c,-s,0,s,c,0,0,0,1);rotDotMatrix.Set(-rate*s,-rate*c,0,rate*c,-rate*s,0,0,0,0);
      rotData=rotMatrix.GetDataVector();rotDotData=rotDotMatrix.GetDataVector();
   }
   void CalculateRotationMatrix(const A1Mjd &epoch,bool) override {rotation(GmatTime(epoch.Get()));}
   void CalculateRotationMatrix(const GmatTime &epoch,bool) override {++preciseCalls;rotation(epoch);}
};
Rvector6 internal(const Rvector6 &local,double theta,double rate) {
   const double c=std::cos(theta),s=std::sin(theta);Rvector6 result;
   result[0]=c*local[0]-s*local[1];result[1]=s*local[0]+c*local[1];result[2]=local[2];
   result[3]=c*local[3]-s*local[4]-rate*result[1];
   result[4]=s*local[3]+c*local[4]+rate*result[0];result[5]=local[5];return result;
}
void unchanged(Spacecraft &sat,const Rvector6 &before) {
   const auto after=sat.GetCartesianState();for(int i=0;i<6;++i)require(after[i]==before[i],"Rejected frame HA operation mutated spacecraft");
}
}

int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   try {
      require((argc==2 || (argc==3 && (QString(argv[2])=="--ordinary" || QString(argv[2])=="--signed")))&&!output.isEmpty(),"Supply startup and evidence directory");
      const bool ordinary=argc==3 && QString(argv[2])=="--ordinary";
      const bool signedCases=argc==3 && QString(argv[2])=="--signed";
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator=Moderator::Instance();require(moderator->Initialize(startup.toStdString(),false),"HA native runtime initialization failed");
      const QString script=output+"/mission.script";
      write(script,QByteArray("Earth.Mu = 398600.4415;\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545.125';\n"
         "Create CoordinateSystem EvalFrame;\nEvalFrame.Origin = Earth;\nEvalFrame.Axes = MJ2000Eq;\n"
         "Create CoordinateSystem NoBody;\nNoBody.Origin = SolarSystemBarycenter;\nNoBody.Axes = MJ2000Eq;\nBeginMissionSequence;\n"));
      require(moderator->InterpretScript(script.toStdString()),"HA frame script did not build");
      auto *sat=dynamic_cast<Spacecraft *>(moderator->GetConfiguredObject("Sat"));
      auto *frame=dynamic_cast<CoordinateSystem *>(moderator->GetConfiguredObject("EvalFrame"));require(sat&&frame,"HA frame/owner missing");
      QJsonArray results;int rejectionChecks=0;
      int vectorRouteChecks=0;
      for (double rate:{0.,.001}) for(bool precise:{false,true}) {
         PrescribedAxes supplied(rate);require(frame->SetRefObject(&supplied,Gmat::AXIS_SYSTEM,supplied.GetName())&&frame->Initialize(),"HA prescribed axes did not bind");
         auto *axes=dynamic_cast<PrescribedAxes *>(frame->GetAxisSystem());require(axes,"HA axes clone missing");
         GmatTime epoch(21545.125);if(precise)epoch.AddSeconds(.0000001);
         sat->SetPrecisionTimeFlag(precise);sat->SetEpochGT(epoch);
         const double theta=.37+rate*(precise?.0000001:0);
         auto *parameter=moderator->CreateParameter("HA","Sat.EvalFrame.HA","Sat","EvalFrame",1);
         require(parameter&&parameter->Initialize()&&parameter->GetCycleType()==GmatParam::NOT_CYCLIC,"HA frame parameter metadata/binding incorrect");
         const std::vector<double> targets=signedCases?std::vector<double>{-400.,-70.,-40.,0.,70.,400.}:
               (ordinary?std::vector<double>{-40.,70.}:std::vector<double>{-40.,70.,400.});
         for(double target:targets) {
            // The parameter converter uses pointer overloads. Check the public
            // Rvector overloads against the same independent R/Rdot oracle.
            const auto local=conic(target),base=internal(local,theta,rate);
            Rvector vectorLocal(6),vectorBase(6),vectorBack(6);
            for(int i=0;i<6;++i)vectorLocal[i]=local[i];
            if(precise) {
               require(axes->RotateToBaseSystem(epoch,vectorLocal,vectorBase)&&
                     axes->RotateFromBaseSystem(epoch,vectorBase,vectorBack),"Precise Rvector rotation failed");
            } else {
               const A1Mjd realEpoch(epoch.GetMjd());
               require(axes->RotateToBaseSystem(realEpoch,vectorLocal,vectorBase)&&
                     axes->RotateFromBaseSystem(realEpoch,vectorBase,vectorBack),"Real Rvector rotation failed");
            }
            for(int i=0;i<6;++i) {
               require(std::abs(vectorBase[i]-base[i])<=(i<3?2e-8:2e-11)+2e-12*std::abs(base[i]),"Rvector forward rotation differs from independent conic/frame");
               require(std::abs(vectorBack[i]-local[i])<=(i<3?2e-8:2e-11)+2e-12*std::abs(local[i]),"Rvector reverse rotation differs from independent conic/frame");
            }
            vectorRouteChecks+=2;
            const int parameterPreciseCalls=axes->preciseCalls;
            const auto initial=internal(conic(20),theta,rate);sat->SetState(initial);
            require(std::abs(parameter->EvaluateReal()-20)<1e-9,"HA frame read lost R or Rdot");
            parameter->SetReal(target);const auto actual=sat->GetCartesianState(),expected=internal(conic(target),theta,rate);
            for(int i=0;i<6;++i) require(std::isfinite(actual[i])&&std::abs(actual[i]-expected[i])<=(i<3?2e-8:2e-11)+2e-12*std::abs(expected[i]),"HA frame write differs from independent R/Rdot conic");
            const double readback=parameter->EvaluateReal();
            if(precise)require(axes->preciseCalls>parameterPreciseCalls,"HA parameter lost precise epoch route independently of vector API checks");
            write(output+"/last-readback.json",QJsonDocument(QJsonObject{{"rate",rate},{"precise",precise},{"target",target},{"readback",readback},{"error",readback-target},{"actual",array(actual)},{"expected",array(expected)}}).toJson());
            require(std::abs(readback-target)<1e-9+2e-12*std::abs(target),"HA frame read-after-write differs");
            results.append(QJsonObject{{"rotation_rate",rate},{"precise_epoch",precise},{"target",target},{"actual",array(actual)},{"expected",array(expected)},{"readback",readback}});
         }
         for(double invalid:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}) {
            const auto before=sat->GetCartesianState();bool rejected=false;try{parameter->SetReal(invalid);}catch(BaseException &){rejected=true;}
            require(rejected,"Frame HA accepted nonfinite input");unchanged(*sat,before);++rejectionChecks;
         }
         sat->SetState(internal(conic(60,false),theta,rate));const auto before=sat->GetCartesianState();
         for(bool reading:{false,true}) {bool rejected=false;try{if(reading)parameter->EvaluateReal();else parameter->SetReal(40);}catch(BaseException &){rejected=true;}
            require(rejected,"Undefined frame conic accepted HA operation");unchanged(*sat,before);++rejectionChecks;}
         if(precise)require(axes->preciseCalls>0,"Frame HA lost precise epoch route");
      }
      sat->SetState(conic(20));auto *invalid=moderator->CreateParameter("HA","Sat.NoBody.HA","Sat","NoBody",1);
      bool initialized=false;try{initialized=invalid&&invalid->Initialize();}catch(BaseException &){}
      for(bool reading:{false,true}) {const auto before=sat->GetCartesianState();bool rejected=!initialized;
         if(initialized)try{if(reading)invalid->EvaluateReal();else invalid->SetReal(40);}catch(BaseException &){rejected=true;}
         require(rejected,"HA accepted a calculated-point origin without Mu");unchanged(*sat,before);++rejectionChecks;}
      const QJsonObject proof{{"status","passed"},{"ordinary_domain_only",ordinary},{"signed_extended_cases",signedCases},{"vector_route_checks",vectorRouteChecks},{"independent_frame_cases",results},{"rejected_atomically",rejectionChecks},
         {"coordinate_system_effects","Independent R and Rdot, inertial/rotating and Real/GmatTime epochs"},{"new_requirement_credit",0}};
      write(output+"/proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}return 1;
}
