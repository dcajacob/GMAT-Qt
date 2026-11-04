#include "StateConversionUtil.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *m){if(!ok)throw std::runtime_error(m);}
static QJsonArray array(const Rvector6 &v){QJsonArray a;for(int i=0;i<6;++i)a.append(v[i]);return a;}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
 require(argc==2,"Expected scalar angular boundary fixture");QFile file(argv[1]);require(file.open(QIODevice::ReadOnly),"Scalar angle fixture missing");const auto fixture=QJsonDocument::fromJson(file.readAll()).object();const auto samples=fixture["samples"].toArray();require(samples.size()==2070,"All2070scalar angle inputs required");constexpr double rad=3.14159265358979323846/180.;QJsonArray records;int checked=0,limited=0,fieldFailures=0,stateFailures=0,unitFailures=0;
 for(const auto &entry:samples){const auto sample=entry.toObject();const auto values=sample["cartesian"].toArray(),expected=sample["expected_angles"].toArray(),held=sample["condition_limited"].toArray();const double mu=sample["mu"].toDouble();Rvector6 cart;for(int i=0;i<6;++i)cart[i]=values[i].toDouble();Rvector3 pos(cart[0],cart[1],cart[2]),vel(cart[3],cart[4],cart[5]);
  const double actual[3]={StateConversionUtil::CartesianToAOP(mu,pos,vel),StateConversionUtil::CartesianToRAAN(mu,pos,vel),StateConversionUtil::CartesianToTA(mu,pos,vel)},radians[3]={StateConversionUtil::CartesianToAOP(mu,pos,vel,true),StateConversionUtil::CartesianToRAAN(mu,pos,vel,true),StateConversionUtil::CartesianToTA(mu,pos,vel,true)};QJsonArray observed,errors,units;
  for(int i=0;i<3;++i){const double error=std::abs(std::remainder(actual[i]-expected[i].toDouble(),360.)),unit=std::abs(std::remainder(actual[i]-radians[i]/rad,360.));observed.append(actual[i]);errors.append(error);units.append(unit);if(held[i].toBool())++limited;else{++checked;if(!std::isfinite(actual[i])||error>1e-8)++fieldFailures;}if(!std::isfinite(radians[i])||unit>2e-12)++unitFailures;}
  auto kep=StateConversionUtil::CartesianToKeplerian(mu,cart,"TA");kep[3]=actual[1];kep[4]=actual[0];kep[5]=actual[2];const auto rebuilt=StateConversionUtil::KeplerianToCartesian(mu,kep,"TA");QJsonArray stateErrors;bool statePass=true;for(int i=0;i<6;++i){const double error=std::abs(rebuilt[i]-cart[i]);stateErrors.append(error);statePass=statePass&&std::isfinite(rebuilt[i])&&error<=(i<3?1e-6:1e-9);}if(!statePass)++stateFailures;
  records.append(QJsonObject{{"index",sample["index"]},{"actual",observed},{"expected",expected},{"angular_errors",errors},{"unit_errors",units},{"condition_limited",held},{"input",values},{"reconstructed",array(rebuilt)},{"state_errors",stateErrors},{"state_passed",statePass}});
 }
 const bool passed=fieldFailures==0&&stateFailures==0&&unitFailures==0;const QJsonObject proof{{"checkpoint","scalar-angle-boundaries"},{"outcome",passed?"passed":"failed"},{"sample_count",2070},{"checked_fields",checked},{"condition_limited_fields",limited},{"failed_fields",fieldFailures},{"failed_states",stateFailures},{"failed_units",unitFailures},{"absolute_tolerance_degrees",1e-8},{"position_tolerance_km",1e-6},{"velocity_tolerance_km_s",1e-9},{"samples",records},{"full_scalar_angle_truth_qualified",false},{"facets",QJsonArray{"scalar-orbital-angle-conventions-and-endpoints","joint-scalar-angle-physical-reconstruction","degree-radian-coherence","ill-conditioned-individual-fields-retained"}},{"scope","2070joint physical reconstructions and5490individual angular fields;720nonzero-near-circular AOP/TA fields remain explicit condition-limited diagnostics,not qualified. Existing circular/equatorial branch thresholds preserved. No general singular/averaging/native input/platform qualification."}};
 const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!directory.isEmpty()){QFile output(QDir(directory).filePath("proof.json"));const auto bytes=QJsonDocument(proof).toJson();require(output.open(QIODevice::WriteOnly)&&output.write(bytes)==bytes.size(),"Scalar angle evidence write failed");}std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
 require(checked==5490&&limited==720,"Scalar angle scope/cardinality changed");require(fieldFailures==0,"Strict individual scalar angle check failed");require(stateFailures==0,"Strict joint scalar angle state reconstruction failed");require(unitFailures==0,"Scalar angle degree/radian mismatch");return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;}
