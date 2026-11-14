#pragma once
#include "FormationAcceptance.hpp"
#include "TimeSystemConverter.hpp"
#include <QDateTime>
#include "Code500EphemerisFile.hpp"
extern "C" {
#include "SpiceUsr.h"
}
namespace plotephem {
inline std::array<double,6> state(double t) {
 return {7000+1.2*t+.003*t*t+1e-6*t*t*t,-2000+4.5*t-.002*t*t+5e-7*t*t*t,500-.7*t+.0004*t*t-2e-7*t*t*t,
         1.2+.006*t+3e-6*t*t,4.5-.004*t+1.5e-6*t*t,-.7+.0008*t-6e-7*t*t};
}
inline QString provider(const QString &name) {
 if(name.startsWith("EphemCode500"))return "Code500";
 if(name.startsWith("EphemStk"))return "STK";
 if(name.startsWith("EphemOem"))return "CCSDS-OEM";
 return "SPK";
}
inline QString makeTable(formaccept::Rig &rig,const QString &name,const QString &method) {
 const bool stk=method=="STK";
 const QString file=name+(stk?".e":".oem");
 const QVector<int> knots{0,31,87,132,194,231,260,283,318,374,451,499,600};
 const auto start=QDateTime::fromString("2000-01-01T11:55:00Z",Qt::ISODate);
 const auto stamp=[&](int offset){return start.addSecs(offset).toUTC().toString("yyyy-MM-dd'T'HH:mm:ss.zzz");};
 QString data;
 if(stk)data="stk.v.10.0\nBEGIN Ephemeris\nNumberOfEphemerisPoints "+QString::number(knots.size())+"\nScenarioEpoch 01 Jan 2000 11:55:00.000\nCentralBody Earth\nCoordinateSystem J2000\nDistanceUnit Kilometers\n\nEphemerisTimePosVel\n\n";
 else data="CCSDS_OEM_VERS = 1.0\nCREATION_DATE = 2000-01-01T12:00:00.000\nORIGINATOR = GMAT acceptance\nMETA_START\nOBJECT_NAME = One\nOBJECT_ID = 2000-001A\nCENTER_NAME = Earth\nREF_FRAME = EME2000\nTIME_SYSTEM = UTC\nSTART_TIME = "+stamp(knots.front())+"\nSTOP_TIME = "+stamp(knots.back())+"\nINTERPOLATION = Lagrange\nINTERPOLATION_DEGREE = 5\nMETA_STOP\n\n";
 // At this epoch TAI-UTC = 32 s and GMAT A1-TAI = 0.0343817 s.
 for(int offset:knots) {
  data+=stk?QString::number(offset):stamp(offset);
  for(double value:state(offset-300+32.0343817))data+=" "+formaccept::number(value);
  data+="\n";
 }
 if(stk)data+="END Ephemeris\n";
 const auto path=rig.files.filePath(file);
 formaccept::write(path,data.toUtf8());rig.save(file,formaccept::read(path));return path;
}
inline QString makeCode500(formaccept::Rig &rig,const QString &name) {
 const QString filename=name+".ephem";
 const QString path=rig.files.filePath(filename);
 Code500EphemerisFile writer(path.toStdString(),123,"UTC","GMAT","Earth",4,2,name.contains("Big")?2:1);
 writer.SetCentralBodyMu(398600.4415);writer.SetTimeIntervalBetweenPoints(20);
 std::array<A1Mjd,31> times;
 std::array<Rvector6,31> states;
 EpochArray epochs;StateArray values;
 for(int i=0;i<31;++i) {
  const double t=20*i-300+32.0343817;
  times[i]=A1Mjd(21545+t/86400.0);
  const auto prescribed=state(t);
  for(int j=0;j<6;++j)states[i][j]=prescribed[j];
  epochs.push_back(&times[i]);values.push_back(&states[i]);
 }
 formaccept::require(writer.WriteDataSegment(epochs,values,true)&&writer.FinalizeHeaders(),"Code500 polynomial fixture write failed");
 writer.CloseForWrite();rig.save(filename,formaccept::read(path));return path;
}
inline QString configureTable(QString setup,formaccept::Rig &rig,const QString &name) {
 const QString method=provider(name);
 const auto file=method=="Code500"?makeCode500(rig,name):makeTable(rig,name,method);
 const auto begin=setup.indexOf("Create Propagator P;"), end=setup.indexOf(name.endsWith("Mixed")?"Create Propagator Q;":"Create OrbitView Orb;");
 formaccept::require(begin>=0&&end>begin,"Tabulated ephemeris setup anchors missing");
 setup.replace(begin,end-begin,"One.EphemerisName = '"+file+"';\nCreate Propagator P;\nP.Type = "+method+";\nP.StepSize = 10;\nP.StartEpoch = 'FromSpacecraft';\n");
 const QStringList fields{"X","Y","Z","VX","VY","VZ"};
 for(int c=0;c<6;++c)setup.replace("One."+fields[c]+" = "+formaccept::number(formaccept::circular(formaccept::orbits[0],0)[c])+";","One."+fields[c]+" = "+formaccept::number(state(0)[c])+";");
 return setup;
}
inline QString configure(QString setup,formaccept::Rig &rig,const QString &name) {
 if(provider(name)!="SPK")return configureTable(setup,rig,name);
 const QString kernel=rig.files.filePath(name+".bsp");
 GmatTime tdb;std::string text;
 TimeSystemConverter::Instance()->Convert("A1ModJulian",GmatTime(21545),"","TDBModJulian",tdb,text);
 const double origin=(tdb.GetMjd()-21545.)*86400.;
 const double knots[]={-1200,-800,-360,0,170,515,920,1400,1800};
 SpiceDouble epochs[9],states[9][6];
 for(int i=0;i<9;++i) {epochs[i]=origin+knots[i];const auto sample=state(knots[i]);for(int j=0;j<6;++j)states[i][j]=sample[j];}
 SpiceInt handle;spkopn_c(kernel.toUtf8().constData(),"Plot endpoint independent cubic",0,&handle);
 if(name.contains("Type9"))spkw09_c(handle,-100055,399,"J2000",epochs[0],epochs[8],"NONUNIFORM CUBIC",3,9,states,epochs);
 else spkw13_c(handle,-100055,399,"J2000",epochs[0],epochs[8],"NONUNIFORM HERMITE",7,9,states,epochs);
 spkcls_c(handle);formaccept::require(!failed_c(),"Polynomial ephemeris fixture creation failed");
 rig.save(name+".bsp",formaccept::read(kernel));
 setup.replace("Orb.CoordinateSystem = EarthMJ2000Eq;", "Orb.CoordinateSystem = EarthICRF;");
 const bool mixed=name.endsWith("Mixed");
 if(mixed)setup.replace("Two.CoordinateSystem = EarthMJ2000Eq;", "Two.CoordinateSystem = EarthICRF;");
 const auto begin=setup.indexOf("Create Propagator P;"),end=setup.indexOf(mixed?"Create Propagator Q;":"Create OrbitView Orb;");
 formaccept::require(begin>=0&&end>begin,"Ephemeris setup anchors missing");
 setup.replace(begin,end-begin,"One.NAIFId = -100055;\nOne.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Propagator P;\nP.Type = SPK;\nP.StepSize = 10;\nP.StartEpoch = 'FromSpacecraft';\n");
 const QStringList fields{"X","Y","Z","VX","VY","VZ"};
 for(int c=0;c<6;++c)setup.replace("One."+fields[c]+" = "+formaccept::number(formaccept::circular(formaccept::orbits[0],0)[c])+";","One."+fields[c]+" = "+formaccept::number(state(0)[c])+";");
 return setup;
}
inline void checkMixedEndpoints(const QJsonArray &records,const QString &method) {
 const QVector<double> ephemeris{0,10,20,23};
 const QVector<double> numerical{0,3,6,9,10,13,16,19,20,23};
 int ephemIndex=0,numericIndex=0;
 for(const auto &value:records) {
  const auto row=value.toObject();
  const bool ephem=row["objects"].toArray()==QJsonArray{"One"};
  const auto &grid=ephem?ephemeris:numerical;
  int &index=ephem?ephemIndex:numericIndex;
  formaccept::require(ephem||row["objects"].toArray()==QJsonArray{"Two"},"Unexpected runtime spacecraft in mixed publication");
  formaccept::require(row["ephemeris"].toBool()==ephem&&row["method"].toString()==(ephem?method:"RungeKutta89"),"Mixed endpoint provider identity differs");
  formaccept::require(index<grid.size()&&std::abs(row["elapsed"].toDouble()-grid[index])<1e-4&&row["initial"].toBool()==(index==0),"Mixed provider endpoint order differs");
  if(method!="SPK") {
   const auto states=row["states"].toArray();
   const auto components=states.size()==1?states.first().toArray():QJsonArray{};
   const auto expected=ephem?state(grid[index]):formaccept::circular(formaccept::orbits[1],grid[index]);
   formaccept::require(components.size()==6,"Missing mixed provider endpoint state");
   for(int i=0;i<6;++i)formaccept::require(std::abs(components[i].toDouble()-expected[i])<(i<3?4e-5:5e-8),"Mixed endpoint differs from independent trajectory");
  }
  ++index;
 }
 formaccept::require(ephemIndex==ephemeris.size()&&numericIndex==numerical.size(),"Mixed provider endpoints missing");
}
}
