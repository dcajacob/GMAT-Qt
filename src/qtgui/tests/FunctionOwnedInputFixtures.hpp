#pragma once
#include "FormationAcceptance.hpp"
#include "Code500EphemerisFile.hpp"
#include "TimeSystemConverter.hpp"
#include "SpiceUsr.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QtEndian>
#include <cstring>
#include <memory>

namespace owned_inputs {
using namespace formaccept;
inline std::array<double,6> affine(bool local,double seconds)
{
   const std::array<double,3> r{local?7000.:9000.,local?-200.:100.,local?450.:300.},v{local?.2:-.1,local?6.8:7.1,local?-.3:.4};
   return {r[0]+v[0]*seconds,r[1]+v[1]*seconds,r[2]+v[2]*seconds,v[0],v[1],v[2]};
}
inline QString checksum(const QString &path)
{
   return QString::fromLatin1(QCryptographicHash::hash(read(path),QCryptographicHash::Sha256).toHex());
}
// Shift the relevant trajectory clock fields of a newly generated one-record
// fixture. State samples are authored at the shifted UTC times below, so this
// preserves the same absolute affine trajectory rather than accepting a grid
// inferred from an incorrectly decoded epoch. No supplied binary is copied.
inline QJsonArray shiftCode500Clock(const QString &path,double shift,int endian)
{
   auto bytes=read(path);QJsonArray recipe;
   for(const auto &field:std::array<std::pair<int,double>,8>{{{40,1},{64,1},{336,1./864},{384,1000},{1536,1./864},{1544,1./864},{5616,1},{8032,1./864}}})
   {
      require(field.first+8<=bytes.size(),"Generated Code500 clock field absent");
      const auto *data=reinterpret_cast<const uchar*>(bytes.constData()+field.first);
      quint64 bits=endian==1?qFromLittleEndian<quint64>(data):qFromBigEndian<quint64>(data);double before;std::memcpy(&before,&bits,8);
      const double after=before+shift*field.second;std::memcpy(&bits,&after,8);auto *output=reinterpret_cast<uchar*>(bytes.data()+field.first);
      if(endian==1)qToLittleEndian<quint64>(bits,output);else qToBigEndian<quint64>(bits,output);
      recipe.append(QJsonObject{{"byte_offset",field.first},{"units_per_second",field.second},{"before",before},{"after",after}});
   }
   write(path,bytes);return recipe;
}
inline QString make(Rig &rig,const QString &directory,const QString &kind,bool local,const QString &repo,double clockShift=0,int fileEndian=1)
{
   require(clockShift==0||kind=="Code500","Clock-shift fixture requires Code500");
   require(fileEndian==1||fileEndian==2,"Unknown input fixture byte order");
   const auto path=directory+(local?"/private-":"/root-")+kind+".eph";
   if(kind=="SPICESGP4")
   {
      const auto source=repo+"/plugins/TLEPropagatorPlugin/test/TLE/TleTestElements.txt";const auto bytes=read(source);write(path,bytes);rig.save("tle-input.txt",bytes);return path;
   }
   auto *time=TimeSystemConverter::Instance();QJsonArray samples;
   std::vector<std::unique_ptr<A1Mjd>> epochs;std::vector<std::unique_ptr<Rvector6>> states;EpochArray epochPointers;StateArray statePointers;
   for(int i=0;i<=8;++i)
   {
      const double gridSeconds=30.*i,seconds=gridSeconds+clockShift;const auto s=affine(local,seconds);QJsonArray values;for(double v:s)values.append(v);samples.append(QJsonObject{{"utc_seconds",seconds},{"state",values}});
      epochs.emplace_back(new A1Mjd(time->Convert(21545.+gridSeconds/86400.,TimeSystemConverter::UTC,TimeSystemConverter::A1)));
      states.emplace_back(new Rvector6(s[0],s[1],s[2],s[3],s[4],s[5]));epochPointers.push_back(epochs.back().get());statePointers.push_back(states.back().get());
   }
   if(kind=="SPK")
   {
      SpiceDouble et[9],data[9][6];const double start=(time->Convert(21545.,TimeSystemConverter::UTC,TimeSystemConverter::TDB)-21545.)*86400.;
      for(int i=0;i<9;++i){et[i]=(time->Convert(21545.+30.*i/86400.,TimeSystemConverter::UTC,TimeSystemConverter::TDB)-21545.)*86400.;const auto s=affine(local,et[i]-start);for(int j=0;j<6;++j)data[i][j]=s[j];}
      SpiceInt handle;spkopn_c(path.toUtf8().constData(),"Scoped owned affine input",0,&handle);spkw09_c(handle,local?-132102:-132101,399,"J2000",et[0],et[8],"SCOPED AFFINE INPUT",1,9,data,et);spkcls_c(handle);require(!failed_c(),"Owned SPK input creation failed");
   }
   else if(kind=="Code500")
   {
      Code500EphemerisFile writer(path.toStdString(),local?132102:132101,"UTC","GMAT","Earth",4,2,fileEndian);writer.SetCentralBodyMu(400000);writer.SetTimeIntervalBetweenPoints(30);writer.SetInitialEpoch(*epochs[0]);writer.SetInitialState(states[0].get());require(writer.WriteDataSegment(epochPointers,statePointers,true)&&writer.FinalizeHeaders(),"Owned Code500 input creation failed");writer.CloseForWrite();
      if(clockShift!=0)rig.save((local?"private-":"root-")+kind+"-clock-shift.json",QJsonDocument(QJsonObject{{"utc_shift_seconds",clockShift},{"file_endian",fileEndian},{"fields",shiftCode500Clock(path,clockShift,fileEndian)}}).toJson());
   }
   else
   {
      const bool stk=kind=="STK";require(stk||kind=="CCSDS-OEM","Unknown owned input format");
      QString text=stk?"stk.v.12.0\nBEGIN Ephemeris\nNumberOfEphemerisPoints 9\nScenarioEpoch 01 Jan 2000 12:00:00.000\nInterpolationMethod Lagrange\nInterpolationOrder 1\nCentralBody Earth\nCoordinateSystem J2000\nDistanceUnit Kilometers\nEphemerisTimePosVel\n":"CCSDS_OEM_VERS = 2.0\nCREATION_DATE = 2000-01-01T00:00:00.000\nORIGINATOR = GMAT\nMETA_START\nOBJECT_NAME = ScopedInput\nOBJECT_ID = ScopedInput\nCENTER_NAME = Earth\nREF_FRAME = EME2000\nTIME_SYSTEM = UTC\nSTART_TIME = 2000-01-01T12:00:00.000\nSTOP_TIME = 2000-01-01T12:04:00.000\nINTERPOLATION = LAGRANGE\nINTERPOLATION_DEGREE = 1\nMETA_STOP\n";
      for(int i=0;i<9;++i){text+=stk?QString::number(30*i):QDateTime(QDate(2000,1,1),QTime(12,0),Qt::UTC).addSecs(30*i).toString("yyyy-MM-ddTHH:mm:ss.zzz");for(double v:affine(local,30.*i))text+=" "+QString::number(v,'g',17);text+='\n';}if(stk)text+="END Ephemeris\n";write(path,text.toUtf8());rig.save((local?"private-":"root-")+kind+"-input.txt",text.toUtf8());
   }
   rig.save((local?"private-":"root-")+kind+"-input-manifest.json",QJsonDocument(QJsonObject{{"kind",kind},{"path",path},{"sha256",checksum(path)},{"samples",samples},{"initial_a1_real",epochs[0]->GetReal()},{"second_a1_real",epochs[1]->GetReal()},{"binary_copies",0},{"kernel_copies",0}}).toJson());return path;
}
}
