#include "STKEphemerisFile.hpp"
#include "CCSDSEphemerisFile.hpp"
#include "CCSDSOEMSegment.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "FileManager.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static QJsonArray values(const Rvector6 &v){QJsonArray a;for(int i=0;i<6;++i)a.append(v[i]);return a;}
static QJsonObject stamp(const GmatTime &t){return{{"day",double(t.GetDays())},{"sec",double(t.GetSec())},{"frac",t.GetFracSec()}};}
static Rvector6 polynomial(int degree,double t)
{
   Rvector6 out;const double r[3]={9000,100,300},v[3]={-.1,7.1,.4};
   for(int i=0;i<3;++i){const double a=degree==3?(i+1)*1e-5:0,b=degree==3?(i-1)*1e-9:0;out[i]=r[i]+v[i]*t+a*t*t+b*t*t*t;out[i+3]=v[i]+2*a*t+3*b*t*t;}return out;
}
static QString epoch(int seconds,const QString &fraction,bool doy=false)
{
   auto date=QDateTime(QDate(2000,1,1),QTime(12,0),Qt::UTC).addSecs(seconds);
   return date.toString(doy?"yyyy-001THH:mm:ss":"yyyy-MM-ddTHH:mm:ss")+"."+fraction;
}
static QString row(const QString &time,int degree,double seconds)
{
   QString text=time;const auto state=polynomial(degree,seconds);for(int i=0;i<6;++i)text+=" "+QString::number(state[i],'g',17);return text+"\n";
}
static void save(const QString &name,const QByteArray &bytes)
{
   const auto dir=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");require(!dir.isEmpty(),"Supply acceptance output directory");
   QFile file(QDir(dir).filePath(name));require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Text-ephemeris evidence write failed");
}
class BaseProbe final:public Ephemeris
{
public:
   BaseProbe(){order=1;useHermite=false;}
   void add(const GmatTime &origin,int start,int end,bool precise,int shift=0)
   {
      Segment segment;
      for(int seconds:{start,end})
      {
         EphemPoint point;point.preciseEpoch=origin;point.preciseEpoch.AddSeconds(seconds);point.theEpoch=point.preciseEpoch.GetMjd();point.hasPrecisionEpoch=precise;point.posvel=polynomial(3,seconds);point.posvel[0]+=shift;segment.points.push_back(point);
      }
      segment.segStart=segment.points.front().theEpoch;segment.segEnd=segment.points.back().theEpoch;theEphem.push_back(segment);a1StartEpoch=theEphem.front().segStart;a1EndEpoch=segment.segEnd;
   }
   void GetInterpolationInfoFromSegment(Integer segment)override{useHermite=segment%2==1;}
};
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);QJsonArray files,observations,controls,baseControls,subUlp;int failures=0;
   try
   {
      require(argc==2,"Supply startup file");auto *manager=FileManager::Instance();manager->ReadStartupFile(argv[1]);
      LeapSecsFileReader leap(manager->GetFullPathname("LEAP_SECS_FILE"));require(leap.Initialize(),"Text-ephemeris leap table failed");auto *time=TimeSystemConverter::Instance();time->SetLeapSecsFileReader(&leap);
      QTemporaryDir directory;require(directory.isValid(),"Text-ephemeris temporary directory failed");
      for(const QString &format:QStringList{"STK","OEM"})for(const QString &method:QStringList{"Lagrange","Hermite"})for(const QString &fraction:QStringList{"000000000","123456789"})for(int degree:{1,3})for(int count:{2,9,17})
      {
         if((degree==3&&count==2)||(format=="OEM"&&method=="Hermite"))continue;
         const QString key=format+"-"+method+"-"+fraction+"-"+QString::number(degree)+"-"+QString::number(count),path=directory.filePath(key+".txt");
         const int end=30*(count-1);const bool stk=format=="STK";
         QString text=stk?"stk.v.12.0\nBEGIN Ephemeris\nNumberOfEphemerisPoints "+QString::number(count)+"\nScenarioEpoch 01 Jan 2000 12:00:00."+fraction+"\nInterpolationMethod "+method+"\nInterpolationOrder "+QString::number(degree)+"\nCentralBody Earth\nCoordinateSystem J2000\nDistanceUnit Kilometers\nEphemerisTimePosVel\n":
            "CCSDS_OEM_VERS = 2.0\nCREATION_DATE = 2000-01-01T00:00:00.000\nORIGINATOR = GMAT\nMETA_START\nOBJECT_NAME = ClockProbe\nOBJECT_ID = ClockProbe\nCENTER_NAME = Earth\nREF_FRAME = EME2000\nTIME_SYSTEM = UTC\nSTART_TIME = "+epoch(0,fraction)+"\nSTOP_TIME = "+epoch(end,fraction)+"\nINTERPOLATION = "+method.toUpper()+"\nINTERPOLATION_DEGREE = "+QString::number(degree)+"\nMETA_STOP\n";
         for(int i=0;i<count;++i)text+=row(stk?QString::number(30*i):epoch(30*i,fraction,i%2),degree,30.*i);if(stk)text+="END Ephemeris\n";
         QFile file(path);require(file.open(QIODevice::WriteOnly)&&file.write(text.toUtf8())>0,"Text-ephemeris fixture write failed");file.close();save(key+"-input.txt",text.toUtf8());
         std::unique_ptr<Ephemeris> ephem;
         if(stk){auto *reader=new STKEphemerisFile;ephem.reset(reader);require(reader->OpenForRead(path.toStdString(),"TimePosVel")&&reader->ReadDataRecords(),"STK fixture read failed");}
         else{auto *reader=new CCSDSEphemerisFile;ephem.reset(reader);require(reader->OpenForRead(path.toStdString(),"TimePosVel"),"OEM fixture read failed");}
         auto origin=time->Convert(GmatTime(21545.),TimeSystemConverter::UTC,TimeSystemConverter::A1);origin.AddSeconds(("0."+fraction).toDouble());
         QJsonArray nodes;for(const auto &segment:ephem->theEphem)for(const auto &p:segment.points)nodes.append(QJsonObject{{"epoch",stamp(p.GetEpoch())},{"precise",p.hasPrecisionEpoch},{"real",p.theEpoch},{"state",values(p.posvel)}});
         files.append(QJsonObject{{"key",key},{"format",format},{"method",method},{"degree",degree},{"count",count},{"fraction",fraction},{"sha256",QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(),QCryptographicHash::Sha256).toHex())},{"origin",stamp(origin)},{"nodes",nodes}});
         std::vector<double> requests{0,.125,14.999999,15,15.000001,30,double(end)/2,double(end)-15,double(end)-.125,double(end)};
         for(int direction:{1,-1})
         {
            if(direction<0)std::reverse(requests.begin(),requests.end());
            for(double seconds:requests)for(const QString &api:QStringList{"Real","GmatTime","RoundedGmatTime"})
            {
               auto query=origin;query.AddSeconds(seconds);
               if(api=="GmatTime"&&seconds==0)query=ephem->theEphem.front().points.front().GetEpoch();
               if(api=="GmatTime"&&seconds==end)query=ephem->theEphem.back().points.back().GetEpoch();
               if(api!="GmatTime")query=GmatTime(query.GetMjd());
               QJsonObject observation{{"file",key},{"direction",direction},{"requested_seconds",seconds},{"api",api},{"query",stamp(query)}};
               try{const auto state=api=="Real"?ephem->InterpolatePoint(query.GetMjd()):ephem->InterpolatePoint(query);observation["state"]=values(state);observation["outcome"]="state";}
               catch(BaseException &e){observation["outcome"]="exception";observation["message"]=QString::fromStdString(e.GetFullMessage());}
               catch(const std::exception &e){observation["outcome"]="std-exception";observation["message"]=e.what();}
               observations.append(observation);
            }
         }
         for(double seconds:{-1e-8,double(end)+1e-8})
         {
            auto query=origin;query.AddSeconds(seconds);QJsonObject observation{{"file",key},{"api","GmatTime"},{"requested_seconds",seconds},{"query",stamp(query)},{"outside",true}};
            try{observation["state"]=values(ephem->InterpolatePoint(query));observation["outcome"]="state";}
            catch(BaseException &e){observation["outcome"]="exception";observation["message"]=QString::fromStdString(e.GetFullMessage());}observations.append(observation);
         }
      }
      // The OEM reader's existing Lagrange-only policy stays enforced.
      const auto invalidPath=directory.filePath("OEM-invalid-Hermite.txt");
      QFile valid(directory.filePath("OEM-Lagrange-123456789-3-17.txt"));require(valid.open(QIODevice::ReadOnly),"OEM negative input missing");
      auto invalid=valid.readAll().replace("INTERPOLATION = LAGRANGE","INTERPOLATION = HERMITE");QFile invalidFile(invalidPath);require(invalidFile.open(QIODevice::WriteOnly)&&invalidFile.write(invalid)>0,"OEM negative input write failed");invalidFile.close();save("OEM-invalid-Hermite-input.txt",invalid);
      bool unsupportedRejected=false;try{CCSDSEphemerisFile reader;reader.OpenForRead(invalidPath.toStdString(),"TimePosVel");}catch(BaseException &e){unsupportedRejected=e.GetFullMessage().find("only supported value")!=std::string::npos;}
      controls.append(QJsonObject{{"unsupported_oem_hermite_rejected",unsupportedRejected}});
      // These adjacent precise epochs intentionally share a rounded MJD.
      CCSDSOEMSegment original;GmatTime first=CCSDSEMSegment::ParseEpochGT("2000-001T12:00:00.123456789"),next=first;next.AddSeconds(1e-8);
      Rvector a(6,1.,2.,3.,4.,5.,6.),b(6,2.,3.,4.,5.,6.,7.);require(original.AddData(first,a)&&original.AddData(next,b)&&original.AddData(next,b),"Precise OEM additions failed");
      bool duplicateRejected=false;try{original.AddData(next,a);}catch(BaseException &){duplicateRejected=true;}
      CCSDSOEMSegment copied(original),assigned;assigned=original;
      for(auto *segment:{&original,&copied,&assigned}){GmatTime read;Rvector data;require(segment->GetEpochAndData(1,read,data),"Copied OEM epoch absent");controls.append(QJsonObject{{"epoch",stamp(read)},{"same_rounded_mjd",first.GetMjd()==next.GetMjd()},{"count",segment->GetNumberOfDataPoints()},{"data",QJsonArray{data[0],data[1],data[2],data[3],data[4],data[5]}},{"duplicate_rejected",duplicateRejected}});}
      const auto before=CCSDSEMSegment::ParseEpochGT("2016-12-31T23:59:59.000000000"),inserted=CCSDSEMSegment::ParseEpochGT("2016-366T23:59:60.000000000"),after=CCSDSEMSegment::ParseEpochGT("2017-01-01T00:00:00.000000000");
      controls.append(QJsonObject{{"leap_before",stamp(before)},{"leap_inserted",stamp(inserted)},{"leap_after",stamp(after)}});
      const auto baseOrigin=first;
      for(bool precise:{false,true})
      {
         BaseProbe probe;probe.add(baseOrigin,0,30,precise);probe.add(baseOrigin,30,60,precise,1000);
         for(int seconds:{15,30,45,60,15,45})
         {
            auto query=baseOrigin;query.AddSeconds(seconds);if(!precise)query=GmatTime(query.GetMjd());
            const int segment=seconds<30?0:1;const auto &points=probe.theEphem[segment].points;QJsonArray nodes;
            for(const auto &p:points)nodes.append(QJsonObject{{"epoch",stamp(p.GetEpoch())},{"state",values(p.posvel)}});
            baseControls.append(QJsonObject{{"precise",precise},{"query",stamp(query)},{"hermite",segment==1},{"nodes",nodes},{"state",values(precise?probe.InterpolatePoint(query):probe.InterpolatePoint(query.GetMjd()))}});
         }
      }
      BaseProbe gap;gap.add(baseOrigin,0,30,true);gap.add(baseOrigin,60,90,true);auto middle=baseOrigin;middle.AddSeconds(45);bool gapRejected=false;try{gap.InterpolatePoint(middle);}catch(BaseException &){gapRejected=true;}
      BaseProbe single;single.add(baseOrigin,0,0,true);auto mutablePoint=single.theEphem.front().points.front();mutablePoint.theEpoch+=1./86400.;
      controls.append(QJsonObject{{"gap_rejected",gapRejected},{"single_state",values(single.InterpolatePoint(baseOrigin))},{"public_epoch_mutation_uses_real",mutablePoint.GetEpoch()==GmatTime(mutablePoint.theEpoch)},{"point_bytes",double(sizeof(Ephemeris::EphemPoint))},{"clock_bytes",double(sizeof(GmatTime))}});
      for(bool stk:{true,false})
      {
         const QString key=stk?"STK-subulp":"OEM-subulp",path=directory.filePath(key+".txt");
         QString text=stk?"stk.v.12.0\nBEGIN Ephemeris\nNumberOfEphemerisPoints 4\nScenarioEpoch 01 Jan 2000 12:00:00.123456789\nInterpolationMethod Lagrange\nInterpolationOrder 1\nCentralBody Earth\nCoordinateSystem J2000\nDistanceUnit Kilometers\nEphemerisTimePosVel\n":
            "CCSDS_OEM_VERS = 2.0\nCREATION_DATE = 2000-01-01T00:00:00.000\nORIGINATOR = GMAT\nMETA_START\nOBJECT_NAME = ClockProbe\nOBJECT_ID = ClockProbe\nCENTER_NAME = Earth\nREF_FRAME = EME2000\nTIME_SYSTEM = UTC\nSTART_TIME = 2000-001T12:00:00.123456789\nSTOP_TIME = 2000-001T12:00:00.123456809\nINTERPOLATION = LAGRANGE\nINTERPOLATION_DEGREE = 1\nMETA_STOP\n";
         for(int i:{0,0,1,2})text+=row(stk?QString::number(i*1e-8,'g',17):epoch(0,QString::number(123456789+10*i),true),1,i*1e-8);if(stk)text+="END Ephemeris\n";
         QFile input(path);require(input.open(QIODevice::WriteOnly)&&input.write(text.toUtf8())>0,"Subulp fixture write failed");input.close();save(key+"-input.txt",text.toUtf8());
         std::unique_ptr<Ephemeris> reader;
         if(stk){auto *p=new STKEphemerisFile;reader.reset(p);require(p->OpenForRead(path.toStdString(),"TimePosVel")&&p->ReadDataRecords(),"STK subulp read failed");}
         else{auto *p=new CCSDSEphemerisFile;reader.reset(p);require(p->OpenForRead(path.toStdString(),"TimePosVel"),"OEM subulp read failed");}
         QJsonArray nodes;bool collapsed=true;const auto &points=reader->theEphem.front().points;
         for(const auto &p:points){nodes.append(QJsonObject{{"epoch",stamp(p.GetEpoch())},{"state",values(p.posvel)}});collapsed=collapsed&&p.theEpoch==points.front().theEpoch;}
         auto query=points.front().GetEpoch();query.AddSeconds(1.5e-8);
         subUlp.append(QJsonObject{{"format",stk?"STK":"OEM"},{"same_rounded_mjd",collapsed},{"nodes",nodes},{"query",stamp(query)},{"state",values(reader->InterpolatePoint(query))},{"sha256",QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(),QCryptographicHash::Sha256).toHex())}});
      }
   }
   catch(BaseException &e){std::cerr<<e.GetFullMessage()<<std::endl;++failures;}
   catch(const std::exception &e){std::cerr<<e.what()<<std::endl;++failures;}
   save("proof.json",QJsonDocument(QJsonObject{{"files",files},{"observations",observations},{"controls",controls},{"base_controls",baseControls},{"subulp_controls",subUlp},{"recording_errors",failures},{"new_requirement_credit",0},{"whole_row_qualified",false}}).toJson());
   std::cout<<"Recorded "<<files.size()<<" files and "<<observations.size()<<" queries"<<std::endl;return failures?1:0;
}
