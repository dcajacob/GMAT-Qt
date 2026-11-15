#include "SatelliteImportData.hpp"
#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QRegularExpression>
#include <QTimeZone>
#include <cmath>
#include <stdexcept>

namespace {
QString quoted(QString text)
{
   return "'"+text.replace("'","''")+"'";
}
QString field(const QJsonObject &object,const QString &name)
{
   const auto value=object[name];
   return value.isDouble() ? QString::number(value.toDouble(),'g',17) : value.toString().trimmed();
}
SatelliteImportData omm(const QJsonObject &object)
{
   SatelliteImportData result;
   result.catalogId=field(object,"NORAD_CAT_ID"); result.name=field(object,"OBJECT_NAME"); result.epoch=field(object,"EPOCH");
   if (result.name.isEmpty()) result.name="Satellite "+result.catalogId;
   bool idValid=false; const auto id=result.catalogId.toLongLong(&idValid);
   if (!idValid || id<=0 || id>999999999) throw std::runtime_error("The orbital record has an invalid catalog number.");
   satelliteEpochGregorian(result.epoch);
   const QMap<QString,QString> metadata{{"CENTER_NAME","EARTH"},{"REF_FRAME","TEME"},{"TIME_SYSTEM","UTC"},{"MEAN_ELEMENT_THEORY","SGP4"}};
   QString text="CCSDS_OMM_VERS = 2.0\nOBJECT_NAME = "+result.name.simplified()+"\nNORAD_CAT_ID = "+result.catalogId+'\n';
   for (auto it=metadata.cbegin();it!=metadata.cend();++it) {
      const auto actual=field(object,it.key());
      if (!actual.isEmpty() && actual.toUpper()!=it.value()) throw std::runtime_error("The orbital record must use Earth-centered TEME/UTC SGP4 elements.");
      text+=it.key()+" = "+it.value()+'\n';
   }
   text+="EPOCH = "+result.epoch+'\n';
   for (const auto &key:QStringList{"MEAN_MOTION","ECCENTRICITY","INCLINATION","RA_OF_ASC_NODE","ARG_OF_PERICENTER","MEAN_ANOMALY","BSTAR","MEAN_MOTION_DOT","MEAN_MOTION_DDOT"}) {
      const auto value=field(object,key); bool valid=false; const double numeric=value.toDouble(&valid);
      if (!valid || !std::isfinite(numeric)) throw std::runtime_error(("Missing or invalid orbital field "+key).toStdString());
      if ((key=="ECCENTRICITY" && (numeric<0 || numeric>=1)) || (key=="MEAN_MOTION" && numeric<=0) ||
          (key=="INCLINATION" && (numeric<0 || numeric>180))) throw std::runtime_error("The orbital record contains an invalid orbit.");
      text+=key+" = "+value+'\n';
      result.meanElements.insert(key,value);
   }
   result.elements=text.toUtf8(); result.extension="omm";
   return result;
}
bool checksum(const QByteArray &line)
{
   if (line.size()!=69 || line[68]<'0' || line[68]>'9') return false;
   int sum=0;
   for (int i=0;i<68;++i) {
      if (line[i]>='0' && line[i]<='9') sum+=line[i]-'0';
      else if (line[i]=='-') ++sum;
   }
   return sum%10==line[68]-'0';
}
}

QString satelliteEpochGregorian(const QString &epoch)
{
   const auto match=QRegularExpression("^(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2}(?:\\.\\d+)?)(?:Z)?$").match(epoch.trimmed());
   if (!match.hasMatch()) throw std::runtime_error("Use a UTC epoch such as 2026-11-12T12:00:00.000Z.");
   const QDate date(match.captured(1).toInt(),match.captured(2).toInt(),match.captured(3).toInt());
   const int hour=match.captured(4).toInt(),minute=match.captured(5).toInt();
   const double second=match.captured(6).toDouble();
   if (!date.isValid() || hour>23 || minute>59 || second<0 || second>=61)
      throw std::runtime_error("The UTC epoch is invalid.");
   return QLocale(QLocale::English).toString(date,"dd MMM yyyy")+' '+match.captured(4)+':'+match.captured(5)+':'+match.captured(6);
}

QVector<SatelliteImportData> parseSatelliteElements(const QByteArray &data)
{
   const auto trimmed=data.trimmed();
   QVector<SatelliteImportData> result;
   if (trimmed.startsWith('[') || trimmed.startsWith('{')) {
      QJsonParseError error; const auto document=QJsonDocument::fromJson(data,&error);
      if (error.error!=QJsonParseError::NoError) throw std::runtime_error("The orbital JSON is invalid.");
      const auto records=document.isArray() ? document.array() : QJsonArray{document.object()};
      for (const auto &record:records) result.append(omm(record.toObject()));
   } else if (trimmed.startsWith("CCSDS_OMM_VERS")) {
      QJsonObject object;
      for (const auto &line:data.split('\n')) {
         const auto separator=line.indexOf('=');
         if (separator>0) object[QString::fromUtf8(line.left(separator)).trimmed()]=QString::fromUtf8(line.mid(separator+1)).trimmed();
      }
      result.append(omm(object));
   } else {
      const auto lines=data.split('\n'); QString name;
      for (int i=0;i<lines.size();++i) {
         auto first=lines[i]; if (first.endsWith('\r')) first.chop(1);
         if (!first.startsWith("1 ")) { if (!first.trimmed().isEmpty()) name=QString::fromUtf8(first).trimmed(); continue; }
         if (++i>=lines.size()) throw std::runtime_error("The TLE is missing its second line.");
         auto second=lines[i]; if (second.endsWith('\r')) second.chop(1);
         if (!second.startsWith("2 ") || !checksum(first) || !checksum(second) || first.mid(2,5)!=second.mid(2,5))
            throw std::runtime_error("The TLE line lengths, checksums or catalog numbers are invalid.");
         bool yearValid=false,dayValid=false;
         const int shortYear=first.mid(18,2).toInt(&yearValid),year=shortYear+(shortYear>=57 ? 1900 : 2000);
         const double day=first.mid(20,12).toDouble(&dayValid);
         if (!yearValid || !dayValid || day<1 || day>=QDate(year,12,31).dayOfYear()+1)
            throw std::runtime_error("The TLE epoch is invalid.");
         QDateTime epoch(QDate(year,1,1),QTime(0,0),QTimeZone(0));
         const qint64 microseconds=qRound64((day-1)*86400000000.0);
         epoch=epoch.addSecs(microseconds/1000000);
         SatelliteImportData value;
         value.catalogId=QString::fromLatin1(first.mid(2,5)).trimmed();
         value.name=name.startsWith("0 ") ? name.mid(2) : name;
         if (value.name.isEmpty()) value.name="Satellite "+value.catalogId;
         value.epoch=epoch.toString("yyyy-MM-ddTHH:mm:ss")+'.'+QString::number(microseconds%1000000).rightJustified(6,'0')+'Z';
         value.elements=value.name.toUtf8()+'\n'+first+'\n'+second+'\n'; value.extension="tle";
         const auto bstar=QString::fromLatin1(first.mid(53,8));
         value.meanElements={{"INCLINATION",QString::fromLatin1(second.mid(8,8)).trimmed()},
            {"RA_OF_ASC_NODE",QString::fromLatin1(second.mid(17,8)).trimmed()},
            {"ECCENTRICITY","0."+QString::fromLatin1(second.mid(26,7)).trimmed()},
            {"ARG_OF_PERICENTER",QString::fromLatin1(second.mid(34,8)).trimmed()},
            {"MEAN_ANOMALY",QString::fromLatin1(second.mid(43,8)).trimmed()},
            {"MEAN_MOTION",QString::fromLatin1(second.mid(52,11)).trimmed()},
            {"BSTAR",(bstar.startsWith('-') ? "-0." : "0.")+bstar.mid(1,5)+"e"+bstar.mid(6,2)}};
         result.append(std::move(value)); name.clear();
      }
   }
   if (result.isEmpty()) throw std::runtime_error("No usable satellite orbital records were found.");
   return result;
}

QString satelliteImportScript(const SatelliteImportData &data,const QString &name,const QString &path)
{
   const QRegularExpression identifier("^[A-Za-z][A-Za-z0-9_]*$");
   if (!identifier.match(name).hasMatch()) throw std::runtime_error("Use a spacecraft name starting with a letter, followed by letters, digits or underscores.");
   if (!identifier.match(data.coordinateSystem).hasMatch() || !identifier.match(data.centralBody).hasMatch())
      throw std::runtime_error("Select a configured coordinate system and central body.");
   satelliteEpochGregorian(data.epoch);
   const auto epoch=data.epoch.trimmed();
   const QDate date=QDate::fromString(epoch.left(10),Qt::ISODate);
   auto secondText=epoch.mid(17); if (secondText.endsWith('Z')) secondText.chop(1);
   const double seconds=secondText.toDouble();
   if (seconds>=60) throw std::runtime_error("Leap-second epochs cannot be represented by this UTC modified-Julian input.");
   const double utcMjd=(date.toJulianDay()-2430000LL)-.5+
      (epoch.mid(11,2).toInt()*3600+epoch.mid(14,2).toInt()*60+seconds)/86400.;
   QString script="Create Spacecraft "+name+";\n";
   // GMAT's Gregorian input limits fractional seconds to milliseconds.
   // UTCModJulian retains the precision supplied by TLE/OMM records.
   script+=name+".DateFormat = UTCModJulian;\n"+name+".Epoch = "+quoted(QString::number(utcMjd,'g',17))+";\n";
   script+=name+".CoordinateSystem = "+data.coordinateSystem+";\n";
   if (data.state) {
      for (int i=0;i<6;++i) {
         const double value=(*data.state)[i];
         if (!std::isfinite(value)) throw std::runtime_error("Every state component must be finite.");
         script+=name+'.'+QStringList{"X","Y","Z","VX","VY","VZ"}[i]+" = "+QString::number(value,'g',17)+";\n";
      }
   } else if (data.propagatorType=="SPK") {
      if (!data.naifId) throw std::runtime_error("Enter the spacecraft NAIF ID from the SPK file.");
      script+=name+".NAIFId = "+QString::number(data.naifId)+";\n";
      script+=name+".OrbitSpiceKernelName = {"+quoted(path)+"};\n";
   } else {
      script+=name+".EphemerisName = "+quoted(path)+";\n";
      if (!data.catalogId.isEmpty()) script+=name+".Id = "+quoted(data.catalogId)+";\n";
   }
   if (!data.propagatorType.isEmpty()) {
      if (!QStringList{"SPICESGP4","STK","CCSDS-OEM","Code500","SPK"}.contains(data.propagatorType))
         throw std::runtime_error("Select a supported ephemeris propagator.");
      script+="Create Propagator "+name+"Prop;\n"+name+"Prop.Type = "+data.propagatorType+";\n";
      if (data.propagatorType=="SPICESGP4") script+=name+"Prop.InitialStepSize = 60;\n";
      else script+=name+"Prop.CentralBody = "+data.centralBody+";\n"+name+"Prop.StartEpoch = 'FromSpacecraft';\n";
   }
   return script;
}
