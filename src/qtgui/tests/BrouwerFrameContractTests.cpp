// Supplementary frame transport and native rejection checks. Original tests
// and numerical bounds remain in their existing workers and fixtures.
#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStateConversion.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

static void require(bool ok, const char *message)
{
   if (!ok) throw std::runtime_error(message);
}

static Rvector6 vector(const QJsonArray &values)
{
   require(values.size()==6, "Expected six oracle components");
   Rvector6 result;
   for (int i=0;i<6;++i) result[i]=values[i].toDouble();
   return result;
}

static Rvector6 ecliptic(const Rvector6 &value)
{
   const double eps=84381.448/3600*std::acos(-1.)/180;
   Rvector6 result=value;
   for (int k:{0,3}) {
      result[k+1]=std::cos(eps)*value[k+1]+std::sin(eps)*value[k+2];
      result[k+2]=-std::sin(eps)*value[k+1]+std::cos(eps)*value[k+2];
   }
   return result;
}

int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("BrouwerFrameContract");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and independent frame fixture");
      TestSettings settings;
      QFile fixture(QFileInfo(argv[2]).absoluteFilePath());require(fixture.open(QIODevice::ReadOnly),"Missing frame oracle");
      const auto cases=QJsonDocument::fromJson(fixture.readAll()).object()["cases"].toArray();
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;require(window.initialize(startup),"Native frame initialization failed");
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");require(source,"Missing script input");
      source->setPlainText("Earth.Mu = 398600.4418;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Ec;\nSat.DisplayStateType = Cartesian;\nBeginMissionSequence;\n");
      require(window.buildScript(),"Native frame fixture build failed");
      auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"));
      auto *frame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthMJ2000Ec"));
      require(sat && frame && frame->AreAxesOfType("InertialAxes"),"Actual inertial frame is missing");
      const double mu=398600.4418,flat=1./298.257223563,radius=6378.1363;
      const A1Mjd epoch(sat->GetEpoch());const GmatTime precise(sat->GetEpoch());
      QJsonArray routes,rejections;
      for (const auto &kind:{QString("Short"),QString("Long")}) {
         QJsonObject spec;
         for (const auto &candidate:cases) if (candidate.toObject()["key"].toString()=="Brouwer"+kind+"EclipticRead") spec=candidate.toObject();
         require(!spec.isEmpty(),"Independent read oracle missing");
         const auto values=spec["expected"].toArray();QJsonArray mean,cart;
         for(int i=0;i<6;++i) {mean.append(values[i]);cart.append(values[i+6]);}
         const auto input=ecliptic(vector(cart)),target=vector(mean);
         const auto type=("BrouwerMean"+kind).toStdString();
         auto check=[&](const Rvector6 &actual,const Rvector6 &expected,bool isMean) {
            for(int i=0;i<6;++i) {
               const double tol=isMean?(i==1?1e-9:1e-6):(i<3?1e-6:1e-9);
               double error=actual[i]-expected[i];
               if(isMean && i>=2) error=std::remainder(error,360.);
               require(std::isfinite(actual[i]) && std::abs(error)<=tol+2e-12*std::abs(expected[i]),"Native frame result differs from independent oracle");
            }
         };
         for (bool usePrecise:{false,true}) {
            const auto actual=usePrecise?OrbitStateConversion::Convert(precise,input,"Cartesian",type,frame,mu,flat,radius):OrbitStateConversion::Convert(epoch,input,"Cartesian",type,frame,mu,flat,radius);
            check(actual,target,true);
            Rvector6 wrapped=target;wrapped[3]+=720;wrapped[4]-=360;wrapped[5]+=1080;
            const auto restored=usePrecise?OrbitStateConversion::Convert(precise,wrapped,type,"Cartesian",frame,mu,flat,radius):OrbitStateConversion::Convert(epoch,wrapped,type,"Cartesian",frame,mu,flat,radius);
            check(restored,input,false);
            routes.append(QJsonObject{{"model",kind},{"precise_epoch",usePrecise},{"read",true},{"wrapped_write",true}});
         }
         const QStringList fields={"SMA","ECC","INC","RAAN","AOP","MA"};
         for (int field=0;field<6;++field) {
            std::vector<double> invalid={std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()};
            if (field==0) {invalid.push_back(0);invalid.push_back(-1);invalid.push_back(1000);}
            if (field==1) {invalid.push_back(1.01);invalid.push_back(-1.01);}
            if (field==2) {invalid.push_back(-1);invalid.push_back(181);}
            for (double bad:invalid) {
               const auto before=sat->GetState();bool rejected=false;std::string diagnostic;
               try {sat->SetRealParameter(("Brouwer"+kind+fields[field]).toStdString(),bad);}
               catch (BaseException &error) {rejected=true;diagnostic=error.GetFullMessage();}
               require(rejected && !diagnostic.empty(),"Invalid native mean write was accepted or lost diagnostic");
               for(int i=0;i<6;++i) require(sat->GetState()[i]==before[i],"Rejected native mean write mutated the state");
               rejections.append(QJsonObject{{"model",kind},{"field",fields[field]},{"finite_input",std::isfinite(bad)},{"state_unchanged",true}});
            }
         }
      }
      require(routes.size()==4 && rejections.size()==50,"Incomplete native frame contract matrix");
      const QJsonObject proof{{"outcome","passed"},{"routes",routes},{"rejections",rejections},{"scope","Two native epoch overloads, two mean models, wrapped writes and fifty rejected scalar writes in EarthMJ2000Ec. Independent NASA-model/basis expectations; no broad frame or mission qualification."}};
      const auto root=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if(!root.isEmpty()) {QFile out(QDir(root).filePath("proof.json"));require(out.open(QIODevice::WriteOnly),"Cannot save native frame proof");out.write(QJsonDocument(proof).toJson());}
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   } catch(BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
   catch(std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
