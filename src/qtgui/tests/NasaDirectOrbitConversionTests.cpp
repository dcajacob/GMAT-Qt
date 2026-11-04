// Direct library dispatch is distinct from the GUI's Cartesian-backed preview.
// Every original GUI test/fixture/assertion remains unchanged.
#include "StateConversionUtil.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
static QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Direct conversion input missing");return f.readAll();}
static Rvector6 vector(const QJsonArray &a) {require(a.size()==6,"Direct conversion needs six components");Rvector6 s;for(int i=0;i<6;++i)s[i]=a[i].toDouble();return s;}
static QJsonArray array(const Rvector6 &s) {QJsonArray a;for(int i=0;i<6;++i)a.append(s[i]);return a;}
static bool angular(const QString &kind,int i) {
 if(kind=="Cartesian")return false;
 if(kind=="Delaunay")return i<3;
 if(kind=="Equinoctial"||kind=="ModifiedEquinoctial"||kind=="AlternateEquinoctial")return i==5;
 if(kind.startsWith("Spherical")||kind=="Planetodetic")return i==1||i==2||i==4||i==5;
 return i>=2;
}
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv);
 try {
  require(argc==3,"Supply direct conversion fixture and repository");const QDir repo(argv[2]);const auto matrix=QJsonDocument::fromJson(read(argv[1])).object();
  const auto references=matrix["reference_sha256"].toObject();for(auto it=references.begin();it!=references.end();++it)require(QString::fromLatin1(QCryptographicHash::hash(read(repo.filePath(it.key())),QCryptographicHash::Sha256).toHex())==it.value().toString(),"Direct conversion oracle provenance changed");
  const auto types=matrix["types"].toArray();require(types.size()==StateConversionUtil::GetTypeCount()&&types.size()==14,"Direct conversion supported type inventory changed");for(int i=0;i<types.size();++i)require(types[i].toString().toStdString()==StateConversionUtil::GetStateTypeList()[i],"Direct conversion type identity changed");
  std::ifstream maps("/proc/self/maps");std::string line;while(std::getline(maps,line)){const auto p=line.find('/');if(p!=std::string::npos)std::cout<<"NASA_LOADED "<<line.substr(p)<<'\n';}
  QJsonArray results;int failures=0,pairs=0,identities=0;
  for(const auto &entry:matrix["cases"].toArray()) {
   const auto spec=entry.toObject();const auto source=spec["source"].toString();const auto input=vector(spec["input"].toArray()),cart=vector(spec["cartesian"].toArray());const double mu=spec["mu"].toDouble(),flat=spec["flattening"].toDouble(),radius=spec["equatorial_radius"].toDouble();
   const auto identity=StateConversionUtil::Convert(input,source.toStdString(),source.toStdString(),mu,flat,radius,"TA");for(int i=0;i<6;++i)require(identity[i]==input[i],"Same-type conversion changed input");++identities;
   for(const auto &target:spec["targets"].toArray()) {
    const auto to=target.toObject();const auto name=to["type"].toString();QJsonObject record{{"case",spec["key"]},{"domain",spec["domain"]},{"source",source},{"target",name},{"input",array(input)},{"expected_cartesian",array(cart)}};bool ok=true;
    try {
     const auto actual=StateConversionUtil::Convert(input,source.toStdString(),name.toStdString(),mu,flat,radius,"TA");record["actual"]=array(actual);
     const auto wanted=to["values"].toArray();QJsonArray fieldErrors;
     for(int i=0;i<6;++i){require(std::isfinite(actual[i]),"Direct conversion returned nonfinite fields");if(wanted.size()==6){double error=actual[i]-wanted[i].toDouble();if(angular(name,i))error=std::remainder(error,360.);fieldErrors.append(error);ok=ok&&std::abs(error)<=matrix["field_tolerance"].toDouble();}}
     record["field_errors"]=fieldErrors;record["oracle"]=to["oracle"];
     const auto reconstructed=StateConversionUtil::Convert(actual,name.toStdString(),"Cartesian",mu,flat,radius,"TA");QJsonArray cartErrors;
     for(int i=0;i<6;++i){const auto error=std::abs(reconstructed[i]-cart[i]);cartErrors.append(error);ok=ok&&std::isfinite(reconstructed[i])&&error<=(i<3?matrix["position_tolerance_km"].toDouble():matrix["velocity_tolerance_km_s"].toDouble());}
     record["reconstructed"]=array(reconstructed);record["cartesian_errors"]=cartErrors;
    } catch(BaseException &error) {ok=false;record["exception"]=QString::fromStdString(error.GetFullMessage());}
    record["passed"]=ok;results.append(record);++pairs;if(!ok){++failures;std::cerr<<"DIRECT_CONVERSION_FAIL "<<source.toStdString()<<" -> "<<name.toStdString()<<" case="<<spec["key"].toString().toStdString()<<'\n';}
   }
  }
  require(pairs==238&&identities==22,"Direct conversion matrix cardinality changed");
  bool badSource=false,badTarget=false;try{StateConversionUtil::Convert(Rvector6(7000,1,2,0,7,1),"MissingType","Cartesian");}catch(BaseException&){badSource=true;}try{StateConversionUtil::Convert(Rvector6(7000,1,2,0,7,1),"Cartesian","MissingType");}catch(BaseException&){badTarget=true;}require(badSource&&badTarget,"Unknown native conversion type accepted");
  const QJsonObject proof{{"checkpoint","direct-orbit-conversion-dispatch"},{"outcome",failures?"failed":"passed"},{"directed_contexts",pairs},{"identity_contexts",identities},{"failed_contexts",failures},{"results",results},{"GUI_executions",0},{"independent_physical_averaging_qualified",false},{"facets",QJsonArray{"actual-public-direct-conversion-dispatch","independent-scalar-and-prototype-forward-physical-reconstruction","all-fourteen-elliptic-types-and-eight-hyperbolic-types","unchanged-identity-and-unknown-type-rejection"}}};
  const auto bytes=QJsonDocument(proof).toJson();const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!directory.isEmpty()){QFile out(QDir(directory).filePath("proof.json"));require(out.open(QIODevice::WriteOnly)&&out.write(bytes)==bytes.size(),"Direct conversion proof write failed");}
  std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';require(failures==0,"Direct native conversion routing/values failed");return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}return 1;
}
