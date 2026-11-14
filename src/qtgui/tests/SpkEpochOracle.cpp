// NAIF-based input oracle. Deliberately does not link GMAT or its time converter.
extern "C" {
#include "SpiceUsr.h"
}
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void check() {if (failed_c()) {char message[1841];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);}}
static QString number(long double v) {return QString::number(static_cast<double>(v),'f',16);}
static QString utcCalendar(double et) {
   char out[128];et2utc_c(et,"C",3,sizeof(out),out);check();const auto fields=QString::fromLatin1(out).simplified().split(' ');
   require(fields.size()==4,"Unexpected NAIF UTC calendar format");QString month=fields[1].toLower();month[0]=month[0].toUpper();return fields[2]+" "+month+" "+fields[0]+" "+fields[3];
}
static QString calendar(long double uniformSeconds) {
   const QDateTime base(QDate(2000,1,1),QTime(12,0),Qt::UTC);
   return QLocale::c().toString(base.addMSecs(std::llround(uniformSeconds*1000)),"dd MMM yyyy HH:mm:ss.zzz");
}
static QJsonObject generate(const QString &lsk) {
   furnsh_c(lsk.toUtf8().constData());check();
   QJsonArray cases;const QStringList names{"J2000","PreLeap","LeapSecond","PostLeap"};
   const QStringList dates{"2000 JAN 01 11:58:00.125 UTC","2016 DEC 31 23:59:58.125 UTC","2016 DEC 31 23:59:60.125 UTC","2017 JAN 01 00:00:00.125 UTC"};
   double centers[4]{};int formats=0,missions=0;
   for (int index=0;index<4;++index) {
      double et;str2et_c(dates[index].toUtf8().constData(),&et);check();centers[index]=et;
      const double tai=unitim_c(et,"TDB","TAI"),tt=unitim_c(et,"TDB","TT");check();require(std::abs((tt-tai)-32.184)<1e-6,"NAIF TT/TAI offset differs from32.184seconds");
      QJsonArray inputs,held;
      for (const auto &scale:QStringList{"A1","TAI","UTC","TT","TDB"}) for (const auto &style:QStringList{"ModJulian","Gregorian"}) {
         const QString format=scale+style;
         if (index==2&&format=="UTCModJulian") {held.append(QJsonObject{{"format",format},{"reason","UTC Modified Julian is ambiguous during a leap second;current NASA guide discourages this representation. No unique ET assertion supplied."}});continue;}
         long double uniform=et;if (scale=="TAI") uniform=tai;else if (scale=="A1") uniform=static_cast<long double>(tai)+0.0343817L;else if (scale=="TT") uniform=tt;
         QString literal;
         if (style=="Gregorian") literal=scale=="UTC"?utcCalendar(et):calendar(uniform);
         else if (scale=="UTC") {
            // This bundled toolkit's JD printer retains only seven fractional
            // day digits. Build MJD arithmetically from its exact .125s UTC
            // calendar instead, avoiding a lossy Julian-date text round trip.
            const auto text=utcCalendar(et);const auto date=QLocale::c().toDateTime(text,"dd MMM yyyy HH:mm:ss.zzz");require(date.isValid(),"NAIF UTC calendar cannot form unique numeric input");
            double back;str2et_c((text+" UTC").toUtf8().constData(),&back);check();require(std::abs(back-et)<=1e-5,"UTC calendar oracle lost numeric input precision");
            const long double mjd=static_cast<long double>(date.date().toJulianDay())-2430000.5L+static_cast<long double>(date.time().msecsSinceStartOfDay())/86400000.0L;literal=number(mjd);
         }
         else literal=number(21545.0L+uniform/86400.0L);
         inputs.append(QJsonObject{{"key",format},{"format",format},{"start_epoch",literal},{"expected_start_et",et},{"epoch_tolerance_seconds",style=="Gregorian"?.0011:.0001},{"kind","explicit"}});++formats;
      }
      inputs.append(QJsonObject{{"key","FromSpacecraft"},{"format","TDBModJulian"},{"start_epoch","FromSpacecraft"},{"expected_start_et",et-240},{"epoch_tolerance_seconds",.0001},{"kind","inherited"}});
      inputs.append(QJsonObject{{"key","EphemStart"},{"format","A1Gregorian"},{"start_epoch","EphemStart"},{"expected_start_et",et-1200},{"epoch_tolerance_seconds",.0001},{"kind","ephemeris-start"}});
      missions+=inputs.size();cases.append(QJsonObject{{"key",names[index]},{"reference_utc",dates[index]},{"center_et",et},{"spacecraft_tdb_mjd",number(21545.0L+(static_cast<long double>(et)-240)/86400.0L)},{"inputs",inputs},{"held_inputs",held}});
   }
   require(formats==39&&missions==47,"Epoch oracle matrix cardinality differs");require(std::abs(centers[3]-centers[1]-3)<1e-5&&std::abs(centers[2]-centers[1]-2)<1e-5,"NAIF2016leap-second intervals differ");
   return QJsonObject{{"schema_version",1},{"oracle","NAIF UTC parsing and uniform TAI/TT/TDB conversion, independent of GMAT converter"},{"gmat_jd_offset",2430000},{"gmat_j2000_mjd",21545},{"A1_minus_TAI_seconds",.0343817},{"TT_minus_TAI_seconds",32.184},{"explicit_format_inputs",formats},{"missions_per_mode",missions},{"cases",cases},{"source_urls",QJsonArray{"https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/unitim_c.html","https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/str2et_c.html","https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/et2utc_c.html"}},{"limits","LSK,NAIF toolkit and declared GMAT A1/MJD conventions are shared external inputs,not independent leap-history or measured-time truth. Millisecond Gregorian and100microsecond numeric epoch bounds are explicit. UTC MJD inside the leap second remains ambiguous/unqualified."}};
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);
   try {
      require(argc==3||argc==4,"Supply LSK and JSON path, optionally --verify");const auto lsk=QFileInfo(argv[1]).absoluteFilePath(),path=QFileInfo(argv[2]).absoluteFilePath();const auto data=generate(lsk);QFile file(path);
      if (argc==4) {require(QString(argv[3])=="--verify"&&file.open(QIODevice::ReadOnly),"Cannot read retained epoch fixture");require(QJsonDocument::fromJson(file.readAll()).object()==data,"Retained epoch input fixture differs from independent NAIF regeneration");}
      else {const auto bytes=QJsonDocument(data).toJson();require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Cannot write epoch fixture");}
      const QJsonObject proof{{"checkpoint","spk-epoch-input-oracle"},{"outcome","passed"},{"explicit_format_inputs",39},{"missions_per_mode",47},{"scenarios",4},{"ambiguous_UTC_MJD_inputs_held",1},{"facets",QJsonArray{"independent-NAIF-input-generation","five-scales-two-formats","2016-leap-second-intervals","ambiguous-UTC-MJD-held"}},{"scope","Input oracle only;no GUI,GMAT execution or measured time validation."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}
}
