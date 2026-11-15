#include "Propagate.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "TimeSystemConverter.hpp"
#include "SpiceOrbitKernelReader.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <array>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
void write(const QString &path,const QByteArray &bytes) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Ephemeris count evidence write failed");}
QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"Ephemeris count output missing");return f.readAll();}
bool build(const QString &source,const QString &directory,const QString &name) {const auto path=directory+"/"+name+".script";write(path,source.toUtf8());return Moderator::Instance()->InterpretScript(path.toStdString());}
Propagate *command() {for(auto *c=Moderator::Instance()->GetFirstCommand();c;c=c->GetNext())if(auto *p=dynamic_cast<Propagate *>(c))return p;return nullptr;}
void counts(const QString &name,const QString &directory,QJsonObject &proof) {
   require(build("Create Propagator A B N;\nA.Type = SPK;\nB.Type = SPK;\nBeginMissionSequence;\n",directory,"configured"),"Ephemeris count owners did not build");
   auto *moderator=Moderator::Instance();auto *a=dynamic_cast<PropSetup *>(moderator->GetConfiguredObject("A")),*b=dynamic_cast<PropSetup *>(moderator->GetConfiguredObject("B")),*n=dynamic_cast<PropSetup *>(moderator->GetConfiguredObject("N"));require(a&&b&&n,"Actual count owners absent");
   std::unique_ptr<PropSetup> equalName(dynamic_cast<PropSetup *>(a->Clone()));require(equalName&&equalName->GetName()==a->GetName(),"Independent same-name setup clone failed");
   const std::array<std::vector<PropSetup*>,7> cases{{{}, {n}, {a}, {a,a}, {a,b}, {n,a}, {a,equalName.get()}}};
   const QStringList policies=name=="CompatibilityCounts"?QStringList{"Native","SingleSetup","SingleGroup"}:QStringList{name.startsWith("ExactlyOneSetup")?"ExactlyOneSetup":"ExactlyOneGroup"};QJsonArray observations;
   for(const auto &policy:policies)for(std::size_t i=0;i<cases.size();++i) {
      const bool native=policy=="Native",group=policy.endsWith("Group"),exact=policy.startsWith("Exactly");const int count=i<2?0:i==2||i==5?1:i==3&&!group?1:2;const bool wanted=native||(exact?count==1:count<=1);
      const auto error=Propagate::EphemerisPolicyError(policy.toStdString(),cases[i]);observations.append(QJsonObject{{"policy",policy},{"shape",int(i)},{"expected_accept",wanted},{"accepted",error.empty()},{"error",QString::fromStdString(error)}});proof["counts"]=observations;require(error.empty()==wanted,"Native count policy differs from independent configured/group cardinality");
   }
   require(!Propagate::EphemerisPolicyError(policies.front().toStdString(),{nullptr}).empty()||policies.front()=="Native","Strict count accepted an unresolved setup");
}
void syntax(const QString &name,const QString &directory,QJsonObject &proof) {
   const QString policy=name.contains("Native")?"Native":name.startsWith("ExactlyOneSetup")?"ExactlyOneSetup":"ExactlyOneGroup";Propagate native;const auto id=native.GetParameterID("EphemerisPolicy");const auto enums=native.GetPropertyEnumStrings(id);
   require(std::find(enums.begin(),enums.end(),policy.toStdString())!=enums.end(),"Exactly-one policy absent from native enum metadata");require(native.SetStringParameter(id,policy.toStdString())&&native.GetStringParameter(id)==policy.toStdString(),"Valid exact policy rejected before its setup references were bound");QJsonArray observations;
   const QString prefix="Create Spacecraft Sat;\nCreate Propagator N;\nBeginMissionSequence;\n";
   for(bool quoted:{false,true}) {
      const QString value=quoted?"'"+policy+"'":policy;const QString statement="Propagate 'Strict label' N(Sat) {EphemerisPolicy = "+value+"};";
      require(build(prefix+statement+"\n",directory,quoted?"quoted":"plain"),"Native exact-policy script syntax did not build");auto *p=command();require(p&&p->GetStringParameter(p->GetParameterID("EphemerisPolicy"))==policy.toStdString(),"Parsed exact policy lost its value");
      p->SetCommentLine("% copied configuration\n");const auto generated=QString::fromStdString(p->GetGeneratingString(Gmat::SCRIPTING));Propagate copied(*p),assigned;assigned.SetName("Destination label");assigned.SetCommentLine("% old destination configuration\n");assigned=*p;
      write(directory+"/copy-metadata-"+(quoted?QString("quoted"):QString("plain"))+".json",QJsonDocument(QJsonObject{{"source_name",QString::fromStdString(p->GetName())},{"copied_name",QString::fromStdString(copied.GetName())},{"assigned_name",QString::fromStdString(assigned.GetName())},{"source_comment",QString::fromStdString(p->GetCommentLine())},{"assigned_comment",QString::fromStdString(assigned.GetCommentLine())}}).toJson());
      for(auto *copy:{&copied,&assigned}){require(copy->GetStringParameter(copy->GetParameterID("EphemerisPolicy"))==policy.toStdString(),"Cold Propagate copy lost exact count policy");require(copy->GetCommentLine()==p->GetCommentLine(),"Cold propagation assignment lost source configuration comments");}
      require(copied.GetName()==p->GetName()&&QString::fromStdString(copied.GetGeneratingString(Gmat::SCRIPTING))==generated,"Cold Propagate constructor lost command metadata or serialization");
      // GmatBase assignment deliberately preserves the destination instance name.
      const auto assignedText=QString(generated).replace("'Strict label'","'Destination label'");require(assigned.GetName()=="Destination label"&&QString::fromStdString(assigned.GetGeneratingString(Gmat::SCRIPTING))==assignedText,"Cold Propagate assignment lost destination identity or source configuration");require(generated.contains(policy)&&generated.contains("Strict label"),"Exact policy serialization lost its label/value");
      require(build(prefix+generated+"\n",directory,quoted?"quoted-roundtrip":"plain-roundtrip"),"Exact policy native serialization did not rebuild");p=command();require(p&&p->GetStringParameter(p->GetParameterID("EphemerisPolicy"))==policy.toStdString(),"Exact policy roundtrip changed mode");observations.append(QJsonObject{{"quoted",quoted},{"generated",generated},{"copy_and_assignment",true}});
   }
   require(build(prefix+"Propagate N(Sat);\n",directory,"omitted"),"Omitted policy script failed");require(command()->GetStringParameter(id)=="Native","Omitted policy did not retain current default");proof["syntax"]=observations;
}
void mission(const QString &name,const QString &directory,QJsonObject &proof) {
   const bool zero=name.endsWith("Zero"),repeated=name.endsWith("Repeated"),mixed=name.endsWith("Mixed"),backward=name.endsWith("Backward"),boundaryStart=name.endsWith("EphemStart")||name.endsWith("DefaultStart");const QString policy=name.contains("Native")?"Native":name.startsWith("ExactlyOneSetup")?"ExactlyOneSetup":"ExactlyOneGroup";
   const auto kernel=directory+"/affine.bsp";SpiceInt handle;spkopn_c(kernel.toUtf8().constData(),"Exact count affine fixture",0,&handle);require(!failed_c(),"Exact count SPK fixture open failed");
   SpiceDouble epochs[]={-120,0,120},states[3][6];for(int i=0;i<3;++i){const double t=epochs[i];const std::array<double,6> state{{7000+t,-100+2*t,200-.5*t,1,2,-.5}};for(int j=0;j<6;++j)states[i][j]=state[j];}spkw09_c(handle,-199993,399,"J2000",-120,120,"AFFINE EARTH",1,3,states,epochs);spkcls_c(handle);require(!failed_c(),"Exact count SPK fixture write failed");
   QString source="Create CoordinateSystem EarthIcrf;\nEarthIcrf.Origin = Earth;\nEarthIcrf.Axes = ICRF;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator N A;\nN.FM = FM;\nN.InitialStepSize = 10;\nN.MinStep = 10;\nN.MaxStep = 10;\nA.Type = SPK;\nA.CentralBody = Earth;\nA.StepSize = 10;\nA.StartEpoch = FromSpacecraft;\nCreate Spacecraft Num E1 E2;\nCreate Variable Keep;\nKeep = 41;\n";
   if(boundaryStart)source.replace("A.StartEpoch = FromSpacecraft;\n",name.endsWith("DefaultStart")?QString():QString("A.StartEpoch = EphemStart;\n"));
   for(const auto &sat:QStringList{"Num","E1","E2"})source+=sat+".CoordinateSystem = EarthIcrf;\n"+sat+".DateFormat = TDBModJulian;\n"+sat+".Epoch = '21545';\n";
   for(const auto &sat:QStringList{"E1","E2"})source+=sat+".NAIFId = -199993;\n"+sat+".OrbitSpiceKernelName = {'"+kernel+"'};\n";
   const QStringList satellites=repeated?QStringList{"E1","E2"}:QStringList{"E1"};
   for(const auto &sat:satellites)source+="Create ReportFile R"+sat+";\nR"+sat+".Filename = '"+directory+"/"+sat+".csv';\nR"+sat+".WriteHeaders = false;\nR"+sat+".FixedWidth = false;\nR"+sat+".Delimiter = ',';\nR"+sat+".Precision = 17;\n";
   const QString groups=zero?"N(Num)":repeated?"A(E1) A(E2)":mixed?"N(Num) A(E1)":"A(E1)";
   source+="BeginMissionSequence;\nPropagate "+QString(backward?"BackProp ":"")+groups+" {EphemerisPolicy = "+policy+"};\nKeep = Keep + 1;\n";
   for(const auto &sat:satellites)source+="Report R"+sat+" "+sat+".A1ModJulian "+sat+".EarthIcrf.X "+sat+".EarthIcrf.Y "+sat+".EarthIcrf.Z "+sat+".EarthIcrf.VX "+sat+".EarthIcrf.VY "+sat+".EarthIcrf.VZ;\n";
   require(build(source,directory,"mission"),"Valid exact-policy syntax was rejected before runtime initialization");if(name.startsWith("Precise")){Moderator::Instance()->GetConfiguredObject("E1")->SetPrecisionTimeFlag(true);Moderator::Instance()->GetConfiguredObject("A")->SetPrecisionTimeFlag(true);}
   proof["configured_precision"]=Moderator::Instance()->GetConfiguredObject("E1")->HasPrecisionTime();proof["ephemeris_start_boundary"]=boundaryStart;const auto status=Moderator::Instance()->RunMission();proof["mission_status"]=status; if(auto *p=command())for(auto *o:p->GetRefObjectArray(Gmat::PROP_SETUP))if(auto *setup=dynamic_cast<PropSetup *>(o))if(setup->GetName()=="A")proof["runtime_provider_precision"]=setup->GetPropagator()->HasPrecisionTime();if(auto *o=Moderator::Instance()->GetInternalObject("E1"))proof["runtime_spacecraft_precision"]=o->HasPrecisionTime();
   if(zero) {require(status!=1,"Exactly-one policy executed a zero-ephemeris mission");auto *p=command();require(p&&!p->IsInitialized(),"Failed exact-count initialization retained readiness");proof["zero_rejected_before_execution"]=true;return;}
   require(status==1,"Exactly-one native SPK mission did not complete");QJsonArray observations;
   for(const auto &sat:satellites) {
      const auto line=QString::fromUtf8(read(directory+"/"+sat+".csv")).trimmed();const auto columns=line.split(',');require(columns.size()==7,"Exact count report cardinality differs");std::array<double,7> actual{};for(int j=0;j<7;++j){bool ok=false;actual[j]=columns[j].trimmed().toDouble(&ok);require(ok&&std::isfinite(actual[j]),"Exact count report contains nonfinite values");}
      const double t=(TimeSystemConverter::Instance()->Convert(actual[0],TimeSystemConverter::A1MJD,TimeSystemConverter::TDBMJD,2430000.)-21545)*86400;const double wantedTime=boundaryStart?-110:backward?-10:10;require(std::abs(t-wantedTime)<2e-5,"Exact count changed native SPK query time");const std::array<double,6> expected{{7000+t,-100+2*t,200-.5*t,1,2,-.5}};double position=0,velocity=0;
      for(int j=0;j<6;++j){const double error=std::abs(actual[j+1]-expected[j]);require(error<(j<3?2e-5:2e-9),"Exact count native SPK output differs from independent affine state");if(j<3)position=std::max(position,error);else velocity=std::max(velocity,error);}observations.append(QJsonObject{{"spacecraft",sat},{"seconds",t},{"position_error_km",position},{"velocity_error_km_s",velocity}});
   }
   require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Exact count mission did not continue");if(mixed){auto *num=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Num"));require(num,"Mixed numerical owner absent");for(int i=0;i<6;++i)require(std::isfinite(num->GetCartesianState()[i]),"Mixed numerical state is nonfinite");proof["mixed_numeric_finite"]=true;}proof["observations"]=observations;
}

void readerCoverageGuards(const QString &directory,QJsonObject &proof) {
   constexpr Integer id=-199994;
   auto kernel=[&](const QString &name,const std::vector<std::array<double,2>> &intervals,double offset) {
      const auto path=directory+"/"+name+".bsp";SpiceInt handle;spkopn_c(path.toUtf8().constData(),"Coverage boundary guards",0,&handle);require(!failed_c(),"Coverage guard kernel open failed");
      for(const auto &interval:intervals){SpiceDouble epochs[]={interval[0],(interval[0]+interval[1])/2,interval[1]},states[3][6];for(int i=0;i<3;++i){const double t=epochs[i];const std::array<double,6> state{{7000+offset+t,-100+2*t,200-.5*t,1,2,-.5}};for(int j=0;j<6;++j)states[i][j]=state[j];}spkw09_c(handle,id,399,"J2000",interval[0],interval[1],"COVERAGE AFFINE EARTH",1,3,states,epochs);require(!failed_c(),"Coverage guard segment write failed");}spkcls_c(handle);require(!failed_c(),"Coverage guard close failed");return path.toStdString();
   };
   SpiceOrbitKernelReader reader;QJsonArray observations;
   auto state=[&](SpiceOrbitKernelReader &r,const auto &epoch,double et,double offset,bool available,const QString &label) {
      bool valid=false;double error=0;QString caught;try {const auto actual=r.GetTargetState("E1",id,epoch,"Earth",399,"ECLIPJ2000");valid=true;for(int j=0;j<6;++j)valid=valid&&std::isfinite(actual[j])&&std::abs(actual[j])<1e100;if(valid){const std::array<double,6> affine{{7000+offset+et,-100+2*et,200-.5*et,1,2,-.5}};std::array<double,6> expected{};SpiceDouble rotation[3][3];pxform_c("J2000","ECLIPJ2000",et,rotation);require(!failed_c(),"Coverage guard oracle rotation failed");for(int block=0;block<2;++block)for(int row=0;row<3;++row)for(int col=0;col<3;++col)expected[3*block+row]+=rotation[row][col]*affine[3*block+col];for(int j=0;j<6;++j)error=std::max(error,std::abs(actual[j]-expected[j]));}}catch(BaseException &e){caught=QString::fromStdString(e.GetFullMessage());}
      observations.append(QJsonObject{{"label",label},{"et",et},{"expected_available",available},{"available",valid},{"state_error",error},{"exception",caught}});proof["observations"]=observations;require(valid==available,"Coverage query changed real outside/gap availability");if(valid)require(error<1e-9,"Coverage query state differs from affine oracle");
   };
   StringArray files{kernel("gapped",{{{-120,-30}},{{30,120}}},0)};RealArray starts,ends;std::vector<GmatTime> startsGT,endsGT;
   reader.GetCoverageSpans(files,id,starts,ends);reader.GetCoverageSpans(files,id,startsGT,endsGT);require(starts.size()==2&&ends.size()==2&&startsGT.size()==2&&endsGT.size()==2,"Coverage enumeration merged a real gap");
   QJsonArray boundaries;
   for(int i=0;i<2;++i)for(bool begin:{true,false}) {
      const double raw=begin?(i?-0.+30:-120):(i?120:-30);const Real epoch=begin?starts[i]:ends[i];const auto precise=begin?startsGT[i]:endsGT[i];const double realET=reader.A1ToSpiceTime(epoch),preciseET=reader.A1ToSpiceTime(precise);
      boundaries.append(QJsonObject{{"raw_et",raw},{"real_et",realET},{"precise_et",preciseET}});proof["boundaries"]=boundaries;require(begin?realET>=raw:realET<=raw,"Real coverage date lies outside the raw interval");require(begin?preciseET>=raw:preciseET<=raw,"Precise coverage date lies outside the raw interval");require(std::abs(realET-raw)<1e-6&&std::abs(preciseET-raw)<1e-10,"Coverage date moved beyond its clock representation");state(reader,A1Mjd(epoch),realET,0,true,"Real boundary");state(reader,precise,preciseET,0,true,"Precise boundary");
   }
   Real first,last;GmatTime firstGT,lastGT;reader.GetCoverageStartAndEnd(files,id,first,last);reader.GetCoverageStartAndEnd(files,id,firstGT,lastGT);require(first==starts.front()&&last==ends.back()&&firstGT==startsGT.front()&&lastGT==endsGT.back(),"Coverage envelope differs from actual interval endpoints");
   for(double et:{-121.,0.,121.,-120.-5e-8,120.+5e-8})state(reader,reader.SpiceTimeToA1GT(et),et,0,false,"Precise gap/outside");
   for(double epoch:{std::nextafter(first,-INFINITY),std::nextafter(last,INFINITY),reader.SpiceTimeToA1(0.)})state(reader,A1Mjd(epoch),reader.A1ToSpiceTime(epoch),0,false,"Real gap/outside");
   SpiceOrbitKernelReader copied(reader),assigned;assigned=reader;state(copied,firstGT,reader.A1ToSpiceTime(firstGT),0,true,"Copy boundary");state(assigned,firstGT,reader.A1ToSpiceTime(firstGT),0,true,"Assignment boundary");
   require(reader.UnloadKernel(files.front()),"Coverage guard unload failed");files={kernel("replacement",{{{200,240}}},500)};require(reader.LoadKernel(files.front()),"Coverage guard replacement load failed");
   for(auto *r:{&reader,&copied,&assigned}){state(*r,firstGT,-120,0,false,"Old pool epoch after replacement");const auto epoch=r->SpiceTimeToA1GT(220);state(*r,epoch,r->A1ToSpiceTime(epoch),500,true,"Replacement pool state");}
   require(reader.UnloadKernel(files.front()),"Replacement pool unload failed");
   const double anchor=reader.SpiceTimeToA1(0.);const double lower=reader.A1ToSpiceTime(anchor),upper=reader.A1ToSpiceTime(std::nextafter(anchor,INFINITY));require(upper>lower,"Native Real clock is not ordered at narrow fixture");const double narrowFirst=lower+(upper-lower)*.25,narrowLast=lower+(upper-lower)*.75;files={kernel("narrow",{{{narrowFirst,narrowLast}}},0)};
   bool legacyRejected=false;try{reader.GetCoverageSpans(files,id,starts,ends);}catch(BaseException &){legacyRejected=true;}require(legacyRejected,"Real clock invented coverage for an interval with no representable date");reader.GetCoverageSpans(files,id,startsGT,endsGT);require(startsGT.size()==1&&endsGT.size()==1,"Precise clock lost narrow coverage");state(reader,startsGT.front(),reader.A1ToSpiceTime(startsGT.front()),0,true,"Narrow precise start");state(reader,endsGT.front(),reader.A1ToSpiceTime(endsGT.front()),0,true,"Narrow precise end");proof["narrow_interval_seconds"]=narrowLast-narrowFirst;proof["legacy_narrow_interval_rejected"]=legacyRejected;
}

void readerPrecision(const QString &directory,QJsonObject &proof) {
   const auto kernel=directory+"/precise-affine.bsp";SpiceInt handle;spkopn_c(kernel.toUtf8().constData(),"Precise reader affine fixture",0,&handle);require(!failed_c(),"Precise reader SPK open failed");
   SpiceDouble epochs[]={-120,0,120},states[3][6];for(int i=0;i<3;++i){const double t=epochs[i];const std::array<double,6> state{{7000+t,-100+2*t,200-.5*t,1,2,-.5}};for(int j=0;j<6;++j)states[i][j]=state[j];}spkw09_c(handle,-199993,399,"J2000",-120,120,"PRECISE AFFINE EARTH",1,3,states,epochs);spkcls_c(handle);require(!failed_c(),"Precise reader SPK write failed");
   SpiceOrbitKernelReader reader;require(reader.LoadKernel(kernel.toStdString()),"Precise reader SPK load failed");QJsonArray observations;
   for(double et:{2e-9,5e-8,1.23456789}) {
      GmatTime tdb(21545.);tdb.AddSeconds(et);const GmatTime a1=TimeSystemConverter::Instance()->Convert(tdb,TimeSystemConverter::TDBMJD,TimeSystemConverter::A1MJD,2430000.);
      const double encoded=reader.A1ToSpiceTime(a1);const auto actual=reader.GetTargetState("E1",-199993,a1,"Earth",399,"ECLIPJ2000");const std::array<double,6> affine{{7000+encoded,-100+2*encoded,200-.5*encoded,1,2,-.5}};std::array<double,6> expected{};SpiceDouble rotation[3][3];pxform_c("J2000","ECLIPJ2000",encoded,rotation);for(int block=0;block<2;++block)for(int row=0;row<3;++row)for(int col=0;col<3;++col)expected[3*block+row]+=rotation[row][col]*affine[3*block+col];double error=0;for(int j=0;j<6;++j)error=std::max(error,std::abs(actual[j]-expected[j]));observations.append(QJsonObject{{"requested_et",et},{"encoded_et",reader.A1ToSpiceTime(a1)},{"mjd_projection",a1.GetMjd()},{"max_state_error",error}});proof["observations"]=observations;require(error<1e-9,"Precise reader discarded time below the absolute-MJD resolution");
   }
}
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QJsonObject proof{{"passed",false}};
   try {require(argc==3&&!directory.isEmpty(),"Supply exact-count startup, case and output");const QString name=argv[2];proof["case"]=name;const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());require(Moderator::Instance()->Initialize(startup.toStdString(),false),"Exact count runtime unavailable");write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Exact count images not acknowledged");if(name=="ReaderCoverageGuards")readerCoverageGuards(directory,proof);else if(name=="ReaderPrecision")readerPrecision(directory,proof);else if(name.endsWith("Counts"))counts(name,directory,proof);else if(name.endsWith("Syntax"))syntax(name,directory,proof);else mission(name,directory,proof);proof["passed"]=true;proof["scope"]="Native ephemeris count, SPK start/clock, affine state and finite coverage/gap/pool/copy guards. All provider/science/domain/capacity/platform conjunctions remain open.";}catch(BaseException &e){proof["error"]=QString::fromStdString(e.GetFullMessage());}catch(std::exception &e){proof["error"]=QString::fromUtf8(e.what());}write(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return proof["passed"].toBool()?0:1;
}
