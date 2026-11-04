#include "SpiceInterface.hpp"
#include "TimeSystemConverter.hpp"
#include "TimeTypes.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
static void require(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 require(argc==2,"Supply canonical epoch contract startup");FileManager::Instance()->ReadStartupFile(argv[1]);SpiceInterface spice;auto *clock=TimeSystemConverter::Instance();const GmatTime origin(21545.0);QJsonArray samples;int probes=0;double maxShift=0,maxResidual=0;bool positive=false,negative=false;
 // Include both sides of ET zero, negative/pre-J2000 ET, day rollover, the
 // actual retained batch epoch, and distant dates. No LSK truth is inferred.
 for(double day:std::array<double,7>{15000,21544,21545,26088,30000,40000,60000})for(double sec:std::array<double,7>{0,1e-9,31.5,43234.0343817,86399.9999999,-1e-7,86400.0000001}){
  GmatTime source(day);source.AddSeconds(sec);const auto original=source;GmatTime canonical(-1);const double encoded=spice.A1ToSpiceTime(source,canonical);require(source==original,"Canonical conversion changed source epoch");require(encoded==spice.A1ToSpiceTime(source),"Canonical overload changed original ET encoding");
  const auto tdb=clock->Convert(canonical,TimeSystemConverter::A1MJD,TimeSystemConverter::TDBMJD,GmatTimeConstants::JD_JAN_5_1941);GmatTime target(origin);target.AddSeconds(encoded);const double residual=std::abs((tdb-target).GetTimeInSec());const double shift=(canonical-source).GetTimeInSec();const double ulp=std::max(std::nextafter(encoded,std::numeric_limits<double>::infinity())-encoded,encoded-std::nextafter(encoded,-std::numeric_limits<double>::infinity()));
  require(residual<2e-11,"Canonical epoch does not represent the encoded ET cell");require(std::abs(shift)<=.500001*ulp+2e-11,"Canonical epoch moved farther than ET rounding permits");require(spice.A1ToSpiceTime(canonical)==encoded,"Canonical epoch re-encodes to a different ET");GmatTime again;require(spice.A1ToSpiceTime(canonical,again)==encoded&&std::abs((again-canonical).GetTimeInSec())<2e-11,"Canonical conversion is not idempotent");
  maxShift=std::max(maxShift,std::abs(shift));maxResidual=std::max(maxResidual,residual);positive|=shift>1e-10;negative|=shift< -1e-10;++probes;
 }
 // The exact grid was captured from the original solved-pass publications.
 GmatTime start(26088.0);start.AddSeconds(43234);start.AddSeconds(.0343817);
 for(int i=0;i<1435;++i){GmatTime source=start;source.AddSeconds(20.*i);GmatTime canonical;double encoded=spice.A1ToSpiceTime(source,canonical);const double delta=(canonical-source).GetTimeInSec();require(std::abs(delta)<3.1e-8,"Batch canonical transport exceeds half an ET cell");samples.append(QJsonObject{{"index",i},{"encoded_et",encoded},{"source_mjd",source.GetMjd()},{"sample_mjd",canonical.GetMjd()},{"delta_seconds",delta},{"sample_days",double(canonical.GetDays())},{"sample_seconds",double(canonical.GetSec())},{"sample_fraction",canonical.GetFracSec()}});}
 require(positive&&negative&&probes==49&&samples.size()==1435,"Canonical coverage lacks both rounding directions or expected grid");
 const QJsonObject result{{"checkpoint","spk-canonical-epoch-contract"},{"outcome","passed"},{"boundary_probes",probes},{"batch_epochs",samples.size()},{"max_boundary_shift_seconds",maxShift},{"max_inverse_residual_seconds",maxResidual},{"samples",samples},{"source_epochs_unchanged",true},{"existing_ET_encoding_unchanged",true},{"scope","Clock primitive only: existing GMAT nonlinear TDB mapping, source isolation, ET-cell bound, idempotence and retained batch grid. No production state sampling, independent clock truth or GUI/SPK physical qualification."}};
 const auto dir=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!dir.isEmpty()){QFile f(dir+"/canonical-epochs.json");require(f.open(QIODevice::WriteOnly),"Cannot preserve canonical epochs");const auto bytes=QJsonDocument(result).toJson();require(f.write(bytes)==bytes.size(),"Incomplete canonical epoch evidence");}QJsonObject proof=result;proof.remove("samples");std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<e.what()<<std::endl;}return 1;}
