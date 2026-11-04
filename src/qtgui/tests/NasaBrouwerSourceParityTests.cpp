// NASA prototype parity is a bounded regression gate, not physical averaging truth.
#include "StateConversionUtil.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static QByteArray read(const QString &path){QFile file(path);require(file.open(QIODevice::ReadOnly),"Brouwer reference file unavailable");return file.readAll();}
static QJsonArray array(const Rvector6 &v){QJsonArray result;for(int i=0;i<6;++i)result.append(v[i]);return result;}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);
 try{
  require(argc==2,"Expected complete Brouwer source-parity corpus");
  const QString fixture=QFileInfo(argv[1]).absoluteFilePath();QDir repository=QFileInfo(fixture).absoluteDir();require(repository.cdUp()&&repository.cdUp()&&repository.cdUp(),"Brouwer repository root unavailable");
  const auto corpus=QJsonDocument::fromJson(read(fixture)).object();const auto sources=corpus["reference_sha256"].toObject();
  for(auto source=sources.begin();source!=sources.end();++source)require(QString::fromLatin1(QCryptographicHash::hash(read(repository.filePath(source.key())),QCryptographicHash::Sha256).toHex())==source.value().toString(),"Brouwer prototype provenance changed;regenerate and review the reference corpus");
  const auto samples=corpus["samples"].toArray();require(samples.size()==60,"All thirty historical reference files at both mu values are required");
  QJsonArray results;int valid=0,invalid=0,failed=0,reopened=0,inverseFailed=0;double maxPosition=0,maxVelocity=0;
  for(const auto &value:samples){const auto sample=value.toObject();const auto input=sample["mean"].toArray();require(input.size()==6,"Brouwer reference needs six fields");
   require(QString::fromLatin1(QCryptographicHash::hash(read(repository.filePath(sample["truth"].toString())),QCryptographicHash::Sha256).toHex())==sample["truth_sha256"].toString(),"Historical Brouwer reference changed");
   Rvector6 mean;for(int i=0;i<6;++i)mean[i]=input[i].toDouble();const double mu=sample["mu"].toDouble();const bool shortPeriod=sample["representation"].toString()=="BrouwerMeanShort";QJsonObject result{{"case",sample["case"]},{"representation",sample["representation"]},{"mu",mu},{"input",input}};
   try{
    auto actual=shortPeriod?StateConversionUtil::BrouwerMeanShortToCartesian(mu,mean):StateConversionUtil::BrouwerMeanLongToCartesian(mu,mean);
    require(!sample["invalid_zero_reference"].toBool(),"Invalid all-zero historical mean reference was accepted");
    const auto expected=sample["expected_cartesian"].toArray();require(expected.size()==6,"Brouwer reference lacks expected state");bool passed=true;double position=0,velocity=0;QJsonArray historicalErrors;
    for(int i=0;i<6;++i){const double error=std::abs(actual[i]-expected[i].toDouble());passed=passed&&std::isfinite(actual[i])&&error<=(i<3?1e-6:1e-9);if(i<3)position=std::max(position,error);else velocity=std::max(velocity,error);historicalErrors.append(std::abs(actual[i]-sample["historical_cartesian"].toArray()[i].toDouble()));}
    const auto back=shortPeriod?StateConversionUtil::CartesianToBrouwerMeanShort(mu,actual):StateConversionUtil::CartesianToBrouwerMeanLong(mu,actual);QJsonArray errors;bool fields=true;for(int i=0;i<6;++i){const double error=i<2?std::abs(back[i]-mean[i]):std::abs(std::remainder(back[i]-mean[i],360.));errors.append(error);fields=fields&&std::isfinite(back[i])&&error<1e-6;}
    const auto reconstructed=shortPeriod?StateConversionUtil::BrouwerMeanShortToCartesian(mu,back):StateConversionUtil::BrouwerMeanLongToCartesian(mu,back);bool inversePassed=true;QJsonArray inverseErrors;for(int i=0;i<6;++i){const double error=std::abs(reconstructed[i]-actual[i]);inverseErrors.append(error);inversePassed=inversePassed&&std::isfinite(reconstructed[i])&&error<=(i<3?1e-6:1e-9);}if(!inversePassed)++inverseFailed;result.insert("reconstructed",array(reconstructed));result.insert("inverse_errors",inverseErrors);result.insert("inverse_passed",inversePassed);
    ++valid;if(!passed)++failed;if(fields)++reopened;maxPosition=std::max(maxPosition,position);maxVelocity=std::max(maxVelocity,velocity);
    result.insert("actual",array(actual));result.insert("expected",expected);result.insert("passed",passed);result.insert("position_error_km",position);result.insert("velocity_error_km_s",velocity);result.insert("historical_FreeFlyer_errors",historicalErrors);result.insert("reopened_mean",array(back));result.insert("reopened_field_errors",errors);result.insert("strict_reopened_diagnostic_pass",fields);
   }catch(BaseException &error){const auto diagnostic=QString::fromStdString(error.GetFullMessage());const bool expected=sample["invalid_zero_reference"].toBool()&&diagnostic.contains(sample["expected_exception"].toString());if(expected)++invalid;else ++failed;result.insert("native_exception",diagnostic);result.insert("passed",expected);}
   results.append(result);
  }
  const auto boundary=QJsonDocument::fromJson(read(repository.filePath("src/qtgui/tests/KeplerBoundaryCases.json"))).object();QJsonArray boundaryResults;int boundaryFailed=0;
  require(boundary["samples"].toArray().size()==2070,"Complete Kepler angular boundary matrix missing");
  for(const auto &value:boundary["samples"].toArray()){const auto sample=value.toObject();const auto expected=sample["cartesian"].toArray();Rvector6 cart;for(int i=0;i<6;++i)cart[i]=expected[i].toDouble();const double mu=sample["mu"].toDouble();const auto elements=StateConversionUtil::CartesianToKeplerian(mu,cart,"TA");const auto actual=StateConversionUtil::KeplerianToCartesian(mu,elements,"TA");QJsonArray errors;bool passed=true;for(int i=0;i<6;++i){const double error=std::abs(actual[i]-cart[i]);errors.append(error);passed=passed&&std::isfinite(actual[i])&&error<=(i<3?1e-6:1e-9);}if(!passed)++boundaryFailed;boundaryResults.append(QJsonObject{{"index",sample["index"]},{"input",expected},{"elements",array(elements)},{"reconstructed",array(actual)},{"errors",errors},{"passed",passed}});}
  const QJsonObject proof{{"checkpoint","brouwer-source-parity"},{"outcome",failed==0&&inverseFailed==0&&boundaryFailed==0?"passed":"failed"},{"samples",results},{"valid_samples",valid},{"invalid_zero_reference_rejections",invalid},{"failed_samples",failed},{"strict_reopened_diagnostic_passes",reopened},{"max_position_error_km",maxPosition},{"max_velocity_error_km_s",maxVelocity},{"position_tolerance_km",1e-6},{"velocity_tolerance_km_s",1e-9},{"inverse_failed_samples",inverseFailed},{"inverse_bounds_km_km_s",QJsonArray{1e-6,1e-9}},{"kepler_boundaries",boundaryResults},{"kepler_boundary_failed_samples",boundaryFailed},{"independent_physical_averaging_qualified",false},{"facets",QJsonArray{"all-retained-brouwer-source-prototype-forward-parity","nonzero-eccentricity-and-plane-preservation","invalid-all-zero-reference-rejection","unqualified-inverse-and-FreeFlyer-diagnostics-retained"}},{"scope","All30historical truth inputs at two mu values;56valid source-prototype forward comparisons and4invalid zero mean references. Historical FreeFlyer and inverse field differences retained diagnostically;not independent physical averaging or all-domain/current NASA qualification."}};
  const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!directory.isEmpty()){QFile file(directory+"/proof.json");const auto bytes=QJsonDocument(proof).toJson();require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Brouwer parity evidence write failed");}
  std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
  require(valid==56&&invalid==4&&failed==0,"Brouwer source parity or invalid reference rejection failed");require(inverseFailed==0,"Strict Brouwer physical inverse reconstruction failed");require(boundaryFailed==0,"Strict Kepler angular-boundary reconstruction failed");return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(const std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
