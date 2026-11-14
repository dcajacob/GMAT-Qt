#include "Code500Propagator.hpp"
#include "NotAKnotInterpolator.hpp"
#include "Spacecraft.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "DateUtil.hpp"
#include "FileManager.hpp"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static QJsonArray values(const Real *data){QJsonArray out;for(int i=0;i<6;++i)out.append(data[i]);return out;}
static QJsonObject stamp(const GmatTime &t){return{{"day",double(t.GetDays())},{"sec",double(t.GetSec())},{"frac",t.GetFracSec()}};}
static std::array<double,6> polynomial(int degree,double t)
{
   const double r[3]={9000,100,300},v[3]={-.1,7.1,.4};std::array<double,6> out;
   for(int i=0;i<3;++i){const double a=degree==3?(i+1)*1e-5:0,b=degree==3?(i-1)*1e-9:0;out[i]=r[i]+v[i]*t+a*t*t+b*t*t*t;out[i+3]=v[i]+2*a*t+3*b*t*t;}return out;
}
class RecordedSpline final:public NotAKnotInterpolator
{
public:
   RecordedSpline():NotAKnotInterpolator("RecordedCode500",6){}
   bool AddPoint(Real time,const Real *data)override{points.append(QJsonObject{{"x_seconds",time},{"state",values(data)}});return NotAKnotInterpolator::AddPoint(time,data);}
   void Clear()override{points=QJsonArray();NotAKnotInterpolator::Clear();}
   QJsonArray points;
};
// The production GetState/UpdateInterpolator implementation runs unchanged.
// This subclass supplies only its file/context fixture and records the actual
// spline inputs; it does not implement the interpolation being verified.
class Probe final:public Code500Propagator
{
public:
   Probe(const QString &path):Code500Propagator("Support"),dummy("SupportSC")
   {
      propObjects.push_back(&dummy);ephemName=path.toStdString();require(ephem.OpenForRead(ephemName)&&ephem.ReadHeader1()&&ephem.ReadHeader2()&&ephem.ReadDataRecords(-999,0),"Code500 support read failed");ephem.GetStartAndEndEpochs(ephemStart,ephemEnd,&ephemRecords);
      for(unsigned i=0;i<ephemRecords->size();++i)
      {
         auto &data=ephemRecords->at(i);const double date=ephem.ReadDoubleField(&data.dateOfFirstEphemPoint_YYYMMDD),sod=ephem.ReadDoubleField(&data.secsOfDayForFirstEphemPoint),step=ephem.ReadDoubleField(&data.timeIntervalBetweenPoints_SEC);const int year=int(date/10000),month=int((date-year*10000)/100),day=int(date-year*10000-month*100);const double raw=ModifiedJulianDate(year+1900,month,day,0,0,sod);const double a1=ephem.GetInputTimeSystem()==1?raw:theTimeConverter->Convert(raw,TimeSystemConverter::UTC,TimeSystemConverter::A1);startEpochs.push_back(a1);timeSteps.push_back(step);timeSpans.push_back(step*50);
         records.append(QJsonObject{{"date",date},{"seconds_of_day",sod},{"step_seconds",step},{"native_a1_start",QString::fromStdString(GetRecordEpoch(i,0).ToString())}});
      }
      interp=new RecordedSpline;ephem.CloseForRead();
   }
   Rvector6 query(const GmatTime &t,bool real){Rvector6 out;if(real)GetState(t.GetMjd(),out);else GetState(t,out);return out;}
   QJsonArray points(){return static_cast<RecordedSpline*>(interp)->points;}
   double start()const{return ephemStart;}double end()const{return ephemEnd;}
   QJsonArray records;
private:
   Spacecraft dummy;
};
static void save(const QJsonObject &proof)
{
   const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(directory.isEmpty())return;QFile f(QDir(directory).filePath("proof.json"));require(f.open(QIODevice::WriteOnly)&&f.write(QJsonDocument(proof).toJson())>0,"Code500 support proof write failed");
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);QJsonArray files,observations;int errors=0;
   try
   {
      require(argc==2,"Supply startup file");auto *manager=FileManager::Instance();manager->ReadStartupFile(argv[1]);LeapSecsFileReader leap(manager->GetFullPathname("LEAP_SECS_FILE"));require(leap.Initialize(),"Code500 support leap table failed");auto *time=TimeSystemConverter::Instance();time->SetLeapSecsFileReader(&leap);QTemporaryDir directory;require(directory.isValid(),"Code500 support temporary directory failed");
      for(const QString &system:QStringList{"A1","UTC"})for(int endian:{1,2})for(int degree:{1,3})for(int count:{9,50,112})
      {
         const QString key=system+"-"+QString::number(endian)+"-"+QString::number(degree)+"-"+QString::number(count),path=directory.filePath(key+".eph");const auto inputSystem=system=="UTC"?TimeSystemConverter::UTC:TimeSystemConverter::A1;const auto origin=time->Convert(GmatTime(21545.),inputSystem,TimeSystemConverter::A1);
         Code500EphemerisFile writer(path.toStdString(),132101,system.toStdString(),"GMAT","Earth",4,2,endian);writer.SetCentralBodyMu(400000);writer.SetTimeIntervalBetweenPoints(30);
         for(int first=0;first<count;first+=50)
         {
            std::vector<std::unique_ptr<A1Mjd>> epochs;std::vector<std::unique_ptr<Rvector6>> states;EpochArray ep;StateArray st;
            for(int i=first;i<std::min(first+50,count);++i){auto stamp=origin;stamp.AddSeconds(30.*i);const auto row=polynomial(degree,30.*i);epochs.emplace_back(new A1Mjd(stamp.GetMjd()));states.emplace_back(new Rvector6(row[0],row[1],row[2],row[3],row[4],row[5]));ep.push_back(epochs.back().get());st.push_back(states.back().get());}
            if(first==0){writer.SetInitialEpoch(*epochs[0]);writer.SetInitialState(states[0].get());}require(writer.WriteDataSegment(ep,st,first+50>=count),"Code500 support fixture write failed");
         }
         require(writer.FinalizeHeaders(),"Code500 support fixture finalize failed");writer.CloseForWrite();QFile input(path);require(input.open(QIODevice::ReadOnly),"Code500 generated input absent");const auto hash=QString::fromLatin1(QCryptographicHash::hash(input.readAll(),QCryptographicHash::Sha256).toHex());input.close();Probe probe(path);
         files.append(QJsonObject{{"key",key},{"system",system},{"endian",endian},{"degree",degree},{"count",count},{"sha256",hash},{"origin_a1_gt",QString::fromStdString(origin.ToString())},{"origin_components",stamp(origin)},{"span_start_real",probe.start()},{"span_end_real",probe.end()},{"records",probe.records}});
         const double end=30.*(count-1);std::vector<double> requests{0,15,30,end/2,end-15,end};if(count>100)requests.insert(requests.end(),{1499.875,1500,1500.125,2999.875,3000});
         for(int direction:{1,-1})
         {
            auto ordered=requests;if(direction<0)std::reverse(ordered.begin(),ordered.end());
            for(double seconds:ordered)for(const QString &api:QStringList{"Real","GmatTime","RoundedGmatTime"})
            {
               auto precise=origin;precise.AddSeconds(seconds);auto query=api=="GmatTime"?precise:GmatTime(precise.GetMjd());if(api=="Real"&&seconds==0)query=GmatTime(probe.start());if(api=="Real"&&seconds==end)query=GmatTime(probe.end());
               QJsonObject row{{"file",key},{"direction",direction},{"api",api},{"requested_seconds",seconds},{"query_a1_gt",QString::fromStdString(query.ToString())},{"query_components",stamp(query)},{"query_real",query.GetMjd()}};
               try{const auto out=probe.query(query,api=="Real");row["actual"]=values(out.GetDataVector());row["rejected"]=false;}catch(BaseException &e){row["rejected"]=true;row["error"]=QString::fromStdString(e.GetFullMessage());row["exception_kind"]="Gmat";}catch(const std::exception &e){row["rejected"]=true;row["error"]=e.what();row["exception_kind"]="std";}
               row["spline_inputs"]=probe.points();observations.append(row);
            }
         }
      }
      time->SetLeapSecsFileReader(nullptr);QJsonObject proof{{"checkpoint","code500-relative-epoch-interpolation"},{"outcome","recorded-for-independent-audit"},{"files",files},{"observations",observations},{"position_bound_km",1e-7},{"velocity_bound_km_s",1e-10},{"runtime_or_kernel_copies",0},{"whole_row_qualified",false},{"new_requirement_credit",0}};save(proof);std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return errors?1:0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   save(QJsonObject{{"outcome","failed"},{"files",files},{"observations",observations}});return 1;
}
