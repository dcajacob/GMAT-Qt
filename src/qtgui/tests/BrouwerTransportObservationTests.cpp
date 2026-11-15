// Supplementary observations for an independent forward-reconstruction audit.
// BrouwerTransportTests.cpp keeps the original strict field contract and its failure.
#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "Parameter.hpp"
#include "AxisSystem.hpp"
#include "CoordinateSystem.hpp"
#include "OrbitStateConversion.hpp"
#include "SpacecraftOrbit.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QCryptographicHash>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Transport input missing");return f.readAll();}
void write(const QString &path,const QByteArray &raw){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(raw)==raw.size(),"Transport output failed");}
Rvector6 vector(const QJsonArray &a){require(a.size()==6,"Six components required");Rvector6 v;for(int i=0;i<6;++i)v[i]=a[i].toDouble();return v;}
QJsonArray array(const Rvector6 &v){QJsonArray a;for(int i=0;i<6;++i)a.append(v[i]);return a;}
class PrescribedTiltedAxes:public AxisSystem {
public:
 double theta,tilt,rate;int preciseCalls=0;
 PrescribedTiltedAxes(double z,double x,double omega):AxisSystem("BrouwerTransportAxes"),theta(z),tilt(x),rate(omega){baseSystem="FK5";}
 GmatBase *Clone()const override{return new PrescribedTiltedAxes(*this);}
 void rotation(const GmatTime &epoch){
  const double angle=theta+rate*(epoch-GmatTime(21545.125)).GetTimeInSec();
  const double c=std::cos(angle),s=std::sin(angle),C=std::cos(tilt),S=std::sin(tilt);
  rotMatrix.Set(c,-s*C,s*S,s,c*C,-c*S,0,S,C);
  rotDotMatrix.Set(-rate*s,-rate*c*C,rate*c*S,rate*c,-rate*s*C,rate*s*S,0,0,0);
  rotData=rotMatrix.GetDataVector();rotDotData=rotDotMatrix.GetDataVector();
 }
 void CalculateRotationMatrix(const A1Mjd &epoch,bool)override{rotation(GmatTime(epoch.Get()));}
 void CalculateRotationMatrix(const GmatTime &epoch,bool)override{++preciseCalls;rotation(epoch);}
};
}
int main(int argc,char **argv)
{
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("BrouwerTransport");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, frozen fixture and repository");TestSettings settings;
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");require(!output.isEmpty(),"Evidence directory required");
  const auto matrix=QJsonDocument::fromJson(read(argv[2])).object();const QDir repo(argv[3]);
  const auto references=matrix["reference_sha256"].toObject();for(auto it=references.begin();it!=references.end();++it)require(QString::fromLatin1(QCryptographicHash::hash(read(repo.filePath(it.key())),QCryptographicHash::Sha256).toHex())==it.value().toString(),"Independent transport oracle provenance changed");
  auto save=[&](const QString &name,const QByteArray &raw){write(QDir(output).filePath(name),raw);};
  QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"Transport runtime failed");
  const QString mission="Earth.Mu = 398600.4418;\nCreate CoordinateSystem EvalFrame;\nEvalFrame.Origin = Earth;\nEvalFrame.Axes = MJ2000Eq;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EvalFrame;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545.125';\nSat.DisplayStateType = Cartesian;\nBeginMissionSequence;\n";
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");require(source,"Transport source missing");source->setPlainText(mission);require(window.buildScript(),"Transport mission Build failed");save("mission.script",mission.toUtf8());
  auto *engine=Moderator::Instance();auto *sat=dynamic_cast<Spacecraft *>(engine->GetConfiguredObject("Sat"));auto *frame=dynamic_cast<CoordinateSystem *>(engine->GetConfiguredObject("EvalFrame"));require(sat&&frame,"Transport owner/frame missing");
  const auto epoch=matrix["epoch"].toDouble();const double mu=matrix["mu"].toDouble(),relative=matrix["relative_tolerance"].toDouble();
  const auto fieldBounds=matrix["mean_field_tolerances"].toArray();const QStringList fields{"SMA","ECC","INC","RAAN","AOP","MA"};
  QJsonArray results,observations;QJsonObject currentSpec;Rvector6 currentPhysical;bool currentPrecise=false;int scalarWrites=0,propertyWrites=0,checks=0;
  auto compare=[&](const Rvector6 &actual,const Rvector6 &expected,bool mean,const QString &phase){
   QJsonArray errors;bool ok=true;
   for(int i=0;i<6;++i){double error=actual[i]-expected[i];if(mean&&i>=2)error=std::remainder(error,360.);const double absolute=mean?fieldBounds[i].toDouble():(i<3?matrix["position_tolerance_km"].toDouble():matrix["velocity_tolerance_km_s"].toDouble());ok=ok&&std::isfinite(actual[i])&&std::abs(error)<=absolute+relative*std::abs(expected[i]);errors.append(error);}
   save("latest-observation.json",QJsonDocument(QJsonObject{{"phase",phase},{"actual",array(actual)},{"expected",array(expected)},{"errors",errors},{"passed",ok}}).toJson());
   if(mean){
    for(int i=0;i<6;++i)require(std::isfinite(actual[i]),"Mean observation is not finite");
    observations.append(QJsonObject{{"case",currentSpec["key"]},{"kind",currentSpec["kind"]},{"theta",currentSpec["theta"]},{"tilt",currentSpec["tilt"]},{"rate",currentSpec["rate"]},{"precise",currentPrecise},{"phase",phase},{"mean",array(actual)},{"literal_expected_mean",array(expected)},{"field_errors",errors},{"strict_field_contract_passed",ok},{"expected_internal",array(currentPhysical)}});
    save("mean-observations.json",QJsonDocument(observations).toJson());
   }else require(ok,"Cartesian write differs from unchanged independent fixed-pole model/R/Rdot oracle");
   ++checks;
  };
  const auto cases=matrix["cases"].toArray();require(cases.size()==36,"Transport matrix cardinality changed");
  for(const auto &entry:cases){const auto spec=entry.toObject();const auto key=spec["key"].toString(),kind=spec["kind"].toString();const auto type="BrouwerMean"+kind;
   const auto earth=vector(spec["earth_osculating"].toArray()),local=vector(spec["local_osculating"].toArray()),mean=vector(spec["local_mean"].toArray());
   for(bool precise:{false,true}){
    currentSpec=spec;currentPhysical=earth;currentPrecise=precise;
    PrescribedTiltedAxes supplied(spec["theta"].toDouble(),spec["tilt"].toDouble(),spec["rate"].toDouble());require(frame->SetRefObject(&supplied,Gmat::AXIS_SYSTEM,supplied.GetName())&&frame->Initialize(),"Prescribed tilted axes failed to bind");auto *axes=dynamic_cast<PrescribedTiltedAxes *>(frame->GetAxisSystem());require(axes,"Tilted axes clone missing");
    sat->SetPrecisionTimeFlag(precise);sat->SetEpochGT(GmatTime(epoch));sat->SetState(earth);
    const auto prefix=key+(precise?"-precise":"-real");
    const auto converted=precise?OrbitStateConversion::Convert(GmatTime(epoch),local,"Cartesian",type.toStdString(),frame,mu,1./298.257223563,6378.1363):OrbitStateConversion::Convert(A1Mjd(epoch),local,"Cartesian",type.toStdString(),frame,mu,1./298.257223563,6378.1363);
    compare(converted,mean,true,prefix+"-helper-read");
    const auto restored=precise?OrbitStateConversion::Convert(GmatTime(epoch),mean,type.toStdString(),"Cartesian",frame,mu,1./298.257223563,6378.1363):OrbitStateConversion::Convert(A1Mjd(epoch),mean,type.toStdString(),"Cartesian",frame,mu,1./298.257223563,6378.1363);
    compare(restored,local,false,prefix+"-helper-write");
    const int beforeSpacecraft=axes->preciseCalls;compare(sat->GetState(type.toStdString()),mean,true,prefix+"-spacecraft-read");if(precise)require(axes->preciseCalls>beforeSpacecraft,"Spacecraft mean read lost precise epoch overload");
    auto *parameter=engine->CreateParameter(type.toStdString(),("Sat.EvalFrame."+type).toStdString(),"Sat","EvalFrame",1);require(parameter&&parameter->Initialize(),"Native mean parameter binding failed");
    compare(parameter->EvaluateRvector6(),mean,true,prefix+"-parameter-read");
    QJsonArray writes;
    for(const auto &value:spec["writes"].toArray()){
     const auto change=value.toObject();const int index=change["field"].toInt();require(index>=0&&index<6,"Invalid field role");sat->SetState(earth);
     auto *scalar=engine->CreateParameter(("Brouwer"+kind+fields[index]).toStdString(),("Sat.EvalFrame.Brouwer"+kind+fields[index]).toStdString(),"Sat","EvalFrame",1);require(scalar&&scalar->Initialize(),"Native scalar binding failed");
     scalar->SetReal(change["input"].toDouble());const auto actual=Rvector6(sat->GetState().GetState());compare(actual,vector(change["expected_internal"].toArray()),false,prefix+"-scalar-"+fields[index]);++scalarWrites;
     currentPhysical=vector(change["expected_internal"].toArray());
     compare(parameter->EvaluateRvector6(),vector(change["local_mean"].toArray()),true,prefix+"-scalar-readback-"+fields[index]);
     writes.append(QJsonObject{{"field",fields[index]},{"input",change["input"]},{"actual_internal",array(actual)},{"expected_internal",change["expected_internal"]}});
    }
    currentPhysical=earth;sat->SetState(earth);QMap<QString,QString> proposed{{"DisplayStateType",type}};
    for(int i=0;i<6;++i)proposed.insert("Brouwer"+kind+fields[i],QString::number(mean[i],'g',17));
    const int beforeProperty=axes->preciseCalls;const auto consumed=applySpacecraftOrbitProperties(*sat,proposed);
    require(consumed.contains("DisplayStateType")&&consumed.size()==7,"Complete mean property write omitted a role");
    compare(Rvector6(sat->GetState().GetState()),earth,false,prefix+"-property-write");++propertyWrites;
    if(precise)require(axes->preciseCalls>beforeProperty,"Complete mean property write lost precise epoch overload");
    require(source->toPlainText()==mission,"Native transport witness changed mission source");
    results.append(QJsonObject{{"case",key},{"precise_epoch",precise},{"local_mean",array(converted)},{"scalar_writes",writes},{"complete_property_write",true},{"precise_axis_calls",axes->preciseCalls}});
    save("results.json",QJsonDocument(results).toJson());
   }
  }
  require(results.size()==72&&scalarWrites==432&&propertyWrites==72&&checks==1224,"Transport effect cardinality differs");
  const QJsonObject proof{{"status","observed;independent-oracle-pending"},{"context_cases",72},{"independent_mean_observations",observations.size()},{"scalar_writes",scalarWrites},{"complete_property_writes",propertyWrites},{"numeric_comparisons",checks},{"results",results},{"new_requirement_credit",0},{"scope",matrix["scope"]}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
