#include "CCSDSOEMWriter.hpp"
#include "EphemWriterCCSDS.hpp"
#include "FileManager.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <array>
#include <clocale>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
using State=std::array<Real,6>;
const State normal{7000.125,-1000.5,2000.25,-1.25,6.5,2.75};
QJsonArray array(const State &s){QJsonArray a;for(auto v:s)a.append(v);return a;}
void save(const QString &path,const QJsonObject &proof){QFile f(path);auto b=QJsonDocument(proof).toJson();require(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"Numeric proof write failed");}
void begin(CCSDSOEMWriter &writer,const QString &path){
 require(writer.OpenFile(path.toStdString()),"Numeric OEM open failed");
 require(writer.SetHeaderForWriting("ORIGINATOR","GMAT numeric-field test")&&writer.WriteHeader("CCSDS_OEM_VERS"),"Numeric OEM header failed");
}
void finish(CCSDSOEMWriter &writer){
 for(const auto &field:{std::make_pair("OBJECT_NAME",std::string("NumericSat")),{"OBJECT_ID","GMAT-Qt-OEM-numeric"},{"CENTER_NAME","EARTH"},{"REF_FRAME","EME2000"},{"TIME_SYSTEM","UTC"},{"START_TIME","2000-01-01T12:00:00.000"},{"STOP_TIME","2000-01-01T12:00:01.000"},{"INTERPOLATION","LAGRANGE"},{"INTERPOLATION_DEGREE","1"}})
  require(writer.SetMetaDataForWriting(field.first,field.second),"Numeric OEM metadata failed");
 require(writer.WriteMetaData()&&writer.WriteDataSegment(),"Numeric OEM segment failed");
}
void add(CCSDSOEMWriter &writer,Real epoch,const State &s){Rvector values(6);for(int i=0;i<6;++i)values[i]=s[i];require(writer.AddDataForWriting(epoch,values),"Numeric valid sample rejected");}
class BufferProbe final:public EphemWriterCCSDS{
public:
 BufferProbe():EphemWriterCCSDS("NumericProbe"){}
 void sample(Real epoch,const Real *state){Real cov[21]{},quat[4]{1,0,0,0},accel[3]{};BufferOrbitData(epoch,state,cov,quat,accel);}
 std::size_t count()const{return a1MjdArray.size();}
 void full(){maxSegmentSize=1;}
 void unfilled(){maxSegmentSize=1000;}
};
}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);
 try{
  require(argc==2,"Supply startup file");const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");require(!directory.isEmpty(),"Supply numeric evidence directory");
  FileManager::Instance()->ReadStartupFile(argv[1]);LeapSecsFileReader leap(FileManager::Instance()->GetFullPathname("LEAP_SECS_FILE"));require(leap.Initialize(),"Numeric leap table failed");
  auto *time=TimeSystemConverter::Instance();time->SetLeapSecsFileReader(&leap);const Real origin=time->Convert(21545.,TimeSystemConverter::UTC,TimeSystemConverter::A1);
  const auto ready=qEnvironmentVariable("GMAT_OEM_MAP_READY");
  if(!ready.isEmpty()){
   QFile marker(ready);require(marker.open(QIODevice::WriteOnly)&&marker.write(QByteArray::number(QCoreApplication::applicationPid()))>0,"Numeric mapping marker failed");marker.close();
   QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_OEM_MAP_ACK"))&&timer.elapsed()<5000)QThread::msleep(5);
   require(QFile::exists(qEnvironmentVariable("GMAT_OEM_MAP_ACK")),"Numeric maps not acknowledged");
  }
  const auto tiny=std::numeric_limits<Real>::denorm_min();
  const std::array<State,3> states{{normal,{std::numeric_limits<Real>::max()/8.,-std::numeric_limits<Real>::max()/8.,std::numeric_limits<Real>::min(),-std::numeric_limits<Real>::min(),tiny,-tiny},{0.,-0.,1e-100,-1e100,123456789.125,-123456789.125}}};
  QJsonArray finite,extended,rejections,buffers;int failures=0;
  const auto comma=qEnvironmentVariable("GMAT_OEM_COMMA_LOCALE","en_DK.utf8");
  for(const auto &profile:QStringList{"C",comma}){
   const auto bytes=profile.toLocal8Bit();require(std::setlocale(LC_NUMERIC,bytes.constData()),"Requested numeric locale unavailable");
   const bool commaDecimal=profile!="C";require(std::string(std::localeconv()->decimal_point)==(commaDecimal?",":"."),"Numeric locale did not select expected decimal separator");
   for(int i=0;i<int(states.size());++i){
    const auto name=QString("finite-%1-%2.oem").arg(commaDecimal?"comma":"C").arg(i);CCSDSOEMWriter writer("1.0");begin(writer,QDir(directory).filePath(name));
    add(writer,origin,states[i]);add(writer,origin+1./86400.,states[i]);finish(writer);
    finite.append(QJsonObject{{"file",name},{"profile",profile},{"decimal_point",QString::fromLatin1(std::localeconv()->decimal_point)},{"state",array(states[i])}});
   }
   {
    const auto name=QString("extended-%1.oem").arg(commaDecimal?"comma":"C");CCSDSOEMWriter writer("2.0");writer.SetUseAcceleration(true);writer.SetUseCovariance(true);begin(writer,QDir(directory).filePath(name));
    Rvector values(30);QJsonArray expected;for(int i=0;i<30;++i){values[i]=i<6?normal[i]:i<27?(i%2?-1.:1.)*(i+.125):i==27?.125:i==28?-.25:.5;expected.append(values[i]);}
    require(writer.AddDataForWriting(origin,values)&&writer.AddDataForWriting(origin+1./86400.,values),"Extended numeric sample rejected");finish(writer);
    extended.append(QJsonObject{{"file",name},{"profile",profile},{"values",expected}});
   }
  }
  require(std::setlocale(LC_NUMERIC,"C"),"Cannot restore numeric locale");
  const std::array<Real,3> invalid{{std::numeric_limits<Real>::quiet_NaN(),std::numeric_limits<Real>::infinity(),-std::numeric_limits<Real>::infinity()}};
  const char *fields[]{"X","Y","Z","X_DOT","Y_DOT","Z_DOT"};
  auto reject=[&](const QString &name,int component,Real invalidValue,int size){
   CCSDSOEMWriter writer("1.0");const auto file=name+".oem";begin(writer,QDir(directory).filePath(file));add(writer,origin,normal);
   Rvector values(size);for(int i=0;i<size;++i)values[i]=normal[i];Real epoch=origin+.5/86400.;if(component>=0)values[component]=invalidValue;else if(component==-1)epoch=invalidValue;
   bool caught=false;std::string error;try{writer.AddDataForWriting(epoch,values);}catch(BaseException &e){caught=true;error=e.GetFullMessage();}
   const auto count=writer.GetNumberOfSerializedDataPoints();const std::string expected=component>=0?fields[component]:component==-1?"EPOCH":"six Cartesian";
   const bool passed=caught&&count==1&&error.find(expected)!=std::string::npos;if(!passed)++failures;
   // Recovery uses the same writer and must export only the two valid samples.
   if(passed){add(writer,origin+1./86400.,normal);require(writer.GetNumberOfSerializedDataPoints()==2,"Rejected numeric sample changed writer count");finish(writer);}
   rejections.append(QJsonObject{{"case",name},{"file",file},{"rejected",caught},{"count_after",count},{"expected_field",QString::fromStdString(expected)},{"error",QString::fromStdString(error)},{"passed",passed}});
  };
  for(int bad=0;bad<3;++bad){reject(QString("epoch-%1").arg(bad),-1,invalid[bad],6);for(int c=0;c<6;++c)reject(QString("state-%1-%2").arg(c).arg(bad),c,invalid[bad],6);}
  for(int size=0;size<6;++size)reject(QString("short-%1").arg(size),-2,0,size);
  for(int bad=0;bad<3;++bad)for(int component=-1;component<6;++component){
   BufferProbe probe;probe.sample(origin,normal.data());probe.full();State state=normal;Real epoch=origin+.5/86400.;if(component<0)epoch=invalid[bad];else state[component]=invalid[bad];
   bool caught=false;std::string error;try{probe.sample(epoch,state.data());}catch(BaseException &e){caught=true;error=e.GetFullMessage();}
   const bool passed=caught&&probe.count()==1;if(!passed)++failures;
   if(passed){probe.unfilled();probe.sample(origin+1./86400.,normal.data());require(probe.count()==2,"Subscriber did not recover after invalid sample");}
   buffers.append(QJsonObject{{"component",component},{"invalid_kind",bad},{"rejected",caught},{"passed",passed},{"error",QString::fromStdString(error)}});
  }
  {BufferProbe probe;probe.sample(origin,normal.data());probe.full();bool caught=false;try{probe.sample(origin+.5/86400.,nullptr);}catch(BaseException &){caught=true;}const bool passed=caught&&probe.count()==1;if(!passed)++failures;buffers.append(QJsonObject{{"null_state",true},{"rejected",caught},{"passed",passed}});}
  save(QDir(directory).filePath("numeric-proof.json"),QJsonObject{{"finite",finite},{"extended",extended},{"writer_rejections",rejections},{"subscriber_rejections",buffers},{"failures",failures},{"new_requirement_credit",0}});
  return failures?1:0;
 }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<e.what()<<'\n';}return 1;
}
