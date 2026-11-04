// The scalar calculation path is distinct from full Keplerian conversion.
#include "StateConversionUtil.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
 require(argc==2,"Expected complete inclination boundary matrix");QFile input(QFileInfo(argv[1]).absoluteFilePath());require(input.open(QIODevice::ReadOnly),"Inclination boundary input missing");const auto fixture=QJsonDocument::fromJson(input.readAll()).object();const auto samples=fixture["samples"].toArray();require(samples.size()==2070,"Complete scalar inclination matrix required");
 constexpr double rad=3.14159265358979323846/180.;QJsonArray results;int failed=0;double maxError=0,maxUnitError=0;
 for(const auto &entry:samples){const auto sample=entry.toObject();const auto expected=sample["keplerian"].toArray(),state=sample["cartesian"].toArray();const double mu=sample["mu"].toDouble();Rvector3 pos(state[0].toDouble(),state[1].toDouble(),state[2].toDouble()),vel(state[3].toDouble(),state[4].toDouble(),state[5].toDouble());
  const double degrees=StateConversionUtil::CartesianToINC(mu,pos,vel,false),radians=StateConversionUtil::CartesianToINC(mu,pos,vel,true),error=std::abs(degrees-expected[2].toDouble()),radError=std::abs(radians-expected[2].toDouble()*rad),unitError=std::abs(degrees-radians/rad);const bool passed=std::isfinite(degrees)&&std::isfinite(radians)&&error<=1e-8&&radError<=1e-8*rad&&unitError<=2e-12;if(!passed)++failed;maxError=std::max(maxError,error);maxUnitError=std::max(maxUnitError,unitError);
  results.append(QJsonObject{{"index",sample["index"]},{"expected_degrees",expected[2]},{"degrees",degrees},{"radians",radians},{"error_degrees",error},{"error_radians",radError},{"unit_error_degrees",unitError},{"passed",passed}});
 }
 QJsonArray rejected;const double mu=1.0;for(const auto &name:{"radial-zero-angular-momentum","parabolic-nonzero-angular-momentum"}){const Rvector3 pos(2,0,0),vel=name==std::string("radial-zero-angular-momentum")?Rvector3(1,0,0):Rvector3(0,1,0);bool rejectedInput=false;QString diagnostic;try{StateConversionUtil::CartesianToINC(mu,pos,vel,false);}catch(BaseException &error){diagnostic=QString::fromStdString(error.GetFullMessage());rejectedInput=!diagnostic.isEmpty();}require(rejectedInput,"Invalid scalar inclination domain was accepted");rejected.append(QJsonObject{{"input",name},{"diagnostic",diagnostic}});}
 const QJsonObject proof{{"checkpoint","scalar-inclination-boundaries"},{"outcome",failed==0?"passed":"failed"},{"samples",results},{"sample_count",samples.size()},{"failed_samples",failed},{"max_error_degrees",maxError},{"max_unit_error_degrees",maxUnitError},{"absolute_tolerance_degrees",1e-8},{"rejections",rejected},{"facets",QJsonArray{"public-scalar-inclination-independent-conic-boundaries","degrees-radians-consistency","invalid-domain-rejection"}},{"scope","2070analytic circular/elliptic/hyperbolic states across prograde/polar/retrograde and near/exact equatorial planes. Radial and parabolic invalid inputs rejected;the existing near-parabolic guard preempts the radial case,so this does not claim separate zero-angular-momentum branch coverage. No all-parameter,propagation/averaging,native input or platform qualification."}};
 const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!directory.isEmpty()){QFile file(QDir(directory).filePath("proof.json"));const auto bytes=QJsonDocument(proof).toJson();require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Scalar inclination evidence write failed");}
 std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;require(failed==0,"Strict scalar inclination boundary or unit check failed");return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(const std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;}
