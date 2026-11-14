#include "TimeSystemConverter.hpp"
#include "LeapSecsFileReader.hpp"
#include "GmatTime.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void check() {if (failed_c()) {char message[1841];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);}}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);
   try {
      require(argc==3,"Supply GMAT leap table and NAIF LSK");LeapSecsFileReader leap(QFileInfo(argv[1]).absoluteFilePath().toStdString());require(leap.Initialize(),"GMAT leap table initialization failed");auto *converter=TimeSystemConverter::Instance();converter->SetLeapSecsFileReader(&leap);furnsh_c(QFileInfo(argv[2]).absoluteFilePath().toUtf8().constData());check();
      QJsonArray observations,rejections;int numeric=0,roundtrips=0;double maxError=0;
      for (const auto &date:QStringList{"31 Dec 2008","30 Jun 2012","30 Jun 2015","31 Dec 2016"}) for (double fraction:{0.,.001,.125,.999}) {
         const QString literal=date+" 23:59:"+QString::number(60+fraction,'f',3);double et;str2et_c((literal+" UTC").toUtf8().constData(),&et);check();QJsonArray values;
         for (const auto &target:QStringList{"TAI","A1","TT","TDB"}) {
            long double expectedSeconds=target=="TDB"?et:unitim_c(et,"TDB",target=="A1"?"TAI":target.toUtf8().constData());check();if (target=="A1") expectedSeconds+=.0343817L;const long double expected=21545.L+expectedSeconds/86400.L;
            for (bool precise:{false,true}) {
               std::string text;double actual;
               if (precise) {GmatTime output;converter->Convert("UTCGregorian",GmatTime(-999.999),literal.toStdString(),(target+"ModJulian").toStdString(),output,text);actual=output.GetMjd();}
               else converter->Convert("UTCGregorian",-999.999,literal.toStdString(),(target+"ModJulian").toStdString(),actual,text);
               const double error=std::abs((static_cast<long double>(actual)-expected)*86400.L);maxError=std::max(maxError,error);if (error>1e-4) std::cerr<<"LEAP_INPUT_DIFF "<<literal.toStdString()<<" target="<<target.toStdString()<<" precise="<<precise<<" error_seconds="<<error<<std::endl;require(std::isfinite(actual)&&error<=1e-4,"Gregorian leap input differs from independent NAIF time");++numeric;values.append(QJsonObject{{"target",target},{"precise",precise},{"actual_mjd",actual},{"expected_mjd",static_cast<double>(expected)},{"error_seconds",error}});
            }
         }
         for (bool precise:{false,true}) for (bool supplyFlag:{false,true}) {
            bool inside=false;std::string text;
            if (precise) {GmatTime output;converter->Convert("UTCGregorian",GmatTime(-999.999),literal.toStdString(),"UTCGregorian",output,text,1,supplyFlag?&inside:nullptr);}
            else {double output;converter->Convert("UTCGregorian",-999.999,literal.toStdString(),"UTCGregorian",output,text,1,supplyFlag?&inside:nullptr);}
            require(QString::fromStdString(text)==literal&&(!supplyFlag||inside),"Gregorian leap input lost60th-second label or flag in round trip");++roundtrips;
         }
         observations.append(QJsonObject{{"input",literal},{"values",values}});
      }
      const QList<QPair<QString,QString>> invalid={{"UTCGregorian","30 Dec 2016 23:59:60.125"},{"UTCGregorian","31 Dec 2016 12:00:60.125"},{"UTCGregorian","01 Jan 2017 00:00:60.125"},{"A1Gregorian","31 Dec 2016 23:59:60.125"},{"TAIGregorian","31 Dec 2016 23:59:60.125"},{"TTGregorian","31 Dec 2016 23:59:60.125"},{"TDBGregorian","31 Dec 2016 23:59:60.125"}};
      for (const auto &input:invalid) for (bool precise:{false,true}) {
         bool rejected=false;std::string text;try {if (precise) {GmatTime output;converter->Convert(input.first.toStdString(),GmatTime(-999.999),input.second.toStdString(),"TAIModJulian",output,text);}else {double output;converter->Convert(input.first.toStdString(),-999.999,input.second.toStdString(),"TAIModJulian",output,text);}}catch (BaseException &) {rejected=true;}
         if (!rejected) std::cerr<<"LEAP_INVALID_ACCEPTED format="<<input.first.toStdString()<<" input="<<input.second.toStdString()<<" precise="<<precise<<std::endl;require(rejected,"Nonexistent or non-UTC60th-second label was accepted");rejections.append(QJsonObject{{"format",input.first},{"input",input.second},{"precise",precise},{"rejected",rejected}});
      }
      require(numeric==128&&roundtrips==64&&rejections.size()==14,"Leap input case cardinality differs");
      const QJsonObject proof{{"checkpoint","leap-second-input"},{"outcome","passed"},{"numeric_checks",numeric},{"roundtrips",roundtrips},{"invalid_rejections",14},{"observations",observations},{"rejections",rejections},{"max_time_error_seconds",maxError},{"facets",QJsonArray{"independent-NAIF-four-leap-dates-four-fractions","Real-and-GmatTime-input-overloads","TAI-A1-TT-TDB-targets","60th-second-roundtrip-with-and-without-flag","nonexistent-and-non-UTC-label-rejection"}},{"scope","Four positive UTC leap insertions2008/2012/2015/2016at fractions.000/.001/.125/.999,Real/GmatTime converters,four uniform targets and UTC identity output. Input/target tolerance100microseconds. Seven invalid date/minute/non-UTC labels checked in both overloads. Shared leap tables/NAIF/conventions,not measured clocks or all historical/future epochs,negative leap seconds,UT1/EOP,ambiguous UTC MJD/native GUI/full NASA qualification."}};
      const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!directory.isEmpty()) {QFile file(QDir(directory).filePath("proof.json"));const auto bytes=QJsonDocument(proof).toJson();require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Leap input proof write failed");}
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
