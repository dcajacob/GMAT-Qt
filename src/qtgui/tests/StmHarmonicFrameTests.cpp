#include "NativeMessageCapture.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStmParameters.hpp"
#include "CoordinateSystem.hpp"
#include "PropSetup.hpp"
#include "ODEModel.hpp"
#include "GravityField.hpp"
#include "OrbitStmFrame.hpp"
#include "CoordinateConverter.hpp"
#include "Propagator.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
void need(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void save(const QString &p,const QByteArray &v){QFile f(p);need(f.open(QIODevice::WriteOnly)&&f.write(v)==v.size(),"STM owner-frame evidence write failed");}
QByteArray read(const QString &p){QFile f(p);need(f.open(QIODevice::ReadOnly),"STM owner-frame fixture missing");return f.readAll();}
QJsonArray matrix(const Rmatrix &m){QJsonArray a;for(int i=0;i<m.GetNumRows();++i){QJsonArray row;for(int j=0;j<m.GetNumColumns();++j)row.append(m(i,j));a.append(row);}return a;}
QJsonArray state(Spacecraft *s){QJsonArray a;for(int i=0;i<6;++i)a.append(s->GetState()[i]);return a;}
QJsonObject snapshot(Spacecraft *s){return QJsonObject{{"state",state(s)},{"stm",matrix(s->GetRmatrixParameter("OrbitSTM"))},{"epoch",s->GetEpoch()},{"precise",s->HasPrecisionTime()}};}
std::unique_ptr<Parameter> parameter(int k,Spacecraft *owner,CoordinateSystem *frame,Moderator *engine){
 const auto prefix=owner->GetName()+"."+frame->GetName()+".";Parameter *value=nullptr;
 switch(k){case 0:value=new OrbitStm(prefix+"OrbitSTM",owner);break;case 1:value=new OrbitStmA(prefix+"OrbitSTMA",owner);break;case 2:value=new OrbitStmB(prefix+"OrbitSTMB",owner);break;case 3:value=new OrbitStmC(prefix+"OrbitSTMC",owner);break;case 4:value=new OrbitStmD(prefix+"OrbitSTMD",owner);break;}
 std::unique_ptr<Parameter> result(value);result->SetSolarSystem(engine->GetSolarSystemInUse());result->SetInternalCoordSystem(owner->GetInternalCoordSystem());result->SetRefObjectName(Gmat::COORDINATE_SYSTEM,frame->GetName());need(result->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,frame->GetName())&&result->Initialize(),"STM owner-dependent Parameter did not bind");return result;
}
}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
 need(argc==3,"Supply STM owner-frame startup and frozen cases");const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");need(!output.isEmpty(),"Supply owner-frame evidence directory");const auto plan=QJsonDocument::fromJson(read(argv[2])).object();
 const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());NativeMessageCapture messages;auto *engine=Moderator::Instance();need(engine->Initialize(startup.toStdString(),false),"STM owner-frame runtime failed");
 const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");if(!ready.isEmpty()){save(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer t;t.start();while(!QFile::exists(ack)&&t.elapsed()<10000)QThread::msleep(5);need(QFile::exists(ack),"STM owner-frame images not acknowledged");}
 const auto potential=QDir(output).filePath("STM-monopole.cof");
 const double fileMu=plan["mu"].toDouble(),fileRadius=plan["potential_radius_km"].toDouble();
 QString cof="COMMENT Synthetic monopole STM provider fixture\n";
 cof+=QString("POTFIELD%1%2  1 %3 %4 1.0\n").arg(2,3).arg(2,3).arg(fileMu*1e9,0,'E',14).arg(fileRadius*1e3,0,'E',14);
 for(int m=0;m<=2;++m)cof+="RECOEF  "+QString::number(2).rightJustified(3)+QString::number(m).rightJustified(3)+"   "+QString::number(m==0 ? -plan["j2"].toDouble()/std::sqrt(5.) : 0.0,'E',14).rightJustified(21)+QString::number(0.0,'E',14).rightJustified(21)+"\n";
 cof+="END\n";save(potential,cof.toUtf8());
 const double mu=plan["mu"].toDouble();QString script="Earth.Mu = "+QString::number(mu,'g',17)+";\nCreate Spacecraft Sat Ref;\n";const QStringList fields{"X","Y","Z","VX","VY","VZ"};
 for(const auto &name:{QString("Sat"),QString("Ref")}){const double r=plan[name=="Sat"?"radius":"reference_radius"].toDouble(),v[]={r,0,0,0,std::sqrt(mu/r),0};script+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545.125';\n";for(int i=0;i<6;++i)script+=name+"."+fields[i]+" = "+QString::number(v[i],'g',17)+";\n";}
 for(const auto &entry:plan["cases"].toArray()){const auto c=entry.toObject();const auto name=c["key"].toString();script+="Create CoordinateSystem "+name+";\n"+name+".Origin = "+c["origin"].toString()+";\n"+name+".Axes = ObjectReferenced;\n"+name+".Primary = "+c["primary"].toString()+";\n"+name+".Secondary = Sat;\n"+name+".XAxis = "+c["x_axis"].toString()+";\n"+name+".ZAxis = N;\n";}
 script+="Create ForceModel Gravity;\nGravity.PrimaryBodies = {Earth};\nGravity.PointMasses = {};\nGravity.GravityField.Earth.PotentialFile = '"+potential+"';\nGravity.GravityField.Earth.Degree = "+QString::number(plan["degree"].toInt())+";\nGravity.GravityField.Earth.Order = "+QString::number(plan["order"].toInt())+";\nGravity.GravityField.Earth.StmLimit = "+QString::number(plan["stm_limit"].toInt())+";\nGravity.Drag = None;\nGravity.SRP = Off;\nGravity.RelativisticCorrection = Off;\nCreate Propagator P;\nP.FM = Gravity;\nP.Type = RungeKutta89;\nP.Accuracy = 1e-13;\nP.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\nBeginMissionSequence;\nPropagate P(Sat, Ref, 'STM') {Sat.ElapsedSecs = 60, StopTolerance = 1e-6};\n";
 const auto path=QDir(output).filePath("mission.script");save(path,script.toUtf8());const bool built=engine->InterpretScript(path.toStdString());save(output+"/build-messages.txt",QByteArray::fromStdString(messages.text));need(built,"STM harmonic mission did not build");messages.text.clear();const int nativeStatus=engine->RunMission();save(output+"/run-messages.txt",QByteArray::fromStdString(messages.text));save(output+"/native-status.json",QJsonDocument(QJsonObject{{"build_status",built},{"run_status",nativeStatus}}).toJson());need(nativeStatus==1,"STM owner-frame harmonic mission failed");
 auto *sat=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat")),*ref=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));need(sat&&ref,"STM owner-frame actual spacecraft missing");
 GravityField *executedField=nullptr;double integration=0;for(auto *command=engine->GetFirstCommand();command;command=command->GetNext())if(command->GetTypeName()=="Propagate"){auto *setup=dynamic_cast<PropSetup*>(command->GetClone(0));need(setup&&setup->GetPropagator(),"STM owner-frame native propagator missing");integration=setup->GetPropagator()->GetTime();auto *fm=setup->GetODEModel();need(fm&&fm->GetNumForces()==1,"Expected one harmonic force provider");auto *field=dynamic_cast<GravityField*>(fm->GetForce(0));executedField=field;need(field&&field->GetTypeName()=="GravityField","Actual harmonic force provider missing");std::string body;Real nativeMu=0;need(field->GetBodyAndMu(body,nativeMu),"Native potential Mu missing");QJsonObject force{{"type",QString::fromStdString(field->GetTypeName())},{"body",QString::fromStdString(body)},{"mu",nativeMu},{"degree",field->GetIntegerParameter("Degree")},{"order",field->GetIntegerParameter("Order")},{"stm_limit",field->GetIntegerParameter("StmLimit")},{"potential",QString::fromStdString(field->GetStringParameter("PotentialFileFullPath"))}};save(output+"/force-provider.json",QJsonDocument(force).toJson());}
 if(plan["degree"].toInt()>0){
  need(executedField,"Executed harmonic field missing");
  auto *input=dynamic_cast<CoordinateSystem*>(executedField->GetRefObject(Gmat::COORDINATE_SYSTEM,executedField->GetStringParameter("InputCoordinateSystem")));
  auto *fixed=dynamic_cast<CoordinateSystem*>(executedField->GetRefObject(Gmat::COORDINATE_SYSTEM,executedField->GetStringParameter("FixedCoordinateSystem")));
  need(input&&fixed&&input->GetBaseSystem()=="FK5"&&input->AreAxesOfType("MJ2000EqAxes"),"Unexpected harmonic input chart");
  OrbitStmFrame::DetachedFrames detached;input=detached.Copy(input);fixed=detached.Copy(fixed);fixed->SetForGravityUpdate(true);
  CoordinateConverter converter;QJsonArray poses;
  for(int k=0;k<=8;++k){const double seconds=integration*k/8;Rvector6 zero(0,0,0,0,0,0),unused;need(converter.Convert(A1Mjd(21545.125+seconds/86400),zero,input,unused,fixed,true,true),"Harmonic pose sample failed");poses.append(QJsonObject{{"seconds",seconds},{"input_to_fixed",matrix(converter.GetLastRotationMatrix())}});}
  save(output+"/force-poses.json",QJsonDocument(QJsonObject{{"samples",poses},{"input_base","FK5"},{"input_axes","MJ2000Eq"},{"sampled_detached_frames",true}}).toJson());
 }
 const QStringList types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"};QJsonArray observations;
 for(const auto &entry:plan["cases"].toArray()){
  const auto key=entry.toObject()["key"].toString();auto *frame=dynamic_cast<CoordinateSystem*>(engine->GetInternalObject(key.toStdString()));need(frame&&frame->GetSecondaryObject()==sat,"Owner frame is not bound to actual target");
  std::vector<std::unique_ptr<Parameter>> original,clones,copies;for(int k=0;k<5;++k){original.push_back(parameter(k,sat,frame,engine));clones.emplace_back(dynamic_cast<Parameter*>(original.back()->Clone()));copies.emplace_back(dynamic_cast<Parameter*>(original.back()->Clone()));copies.back()->Copy(original.back().get());}
  for(const auto &provider:{QString("original"),QString("clone"),QString("copy")})for(int k=0;k<5;++k){auto *p=provider=="original"?original[k].get():provider=="clone"?clones[k].get():copies[k].get();need(p,"STM owner-frame Parameter clone missing");const auto before=snapshot(sat),guideBefore=snapshot(ref);const auto cache=matrix(p->GetRmatrix());bool rejected=false;QString diagnostic;QJsonArray actual;
   try{actual=matrix(p->EvaluateRmatrix());}catch(BaseException &e){rejected=true;diagnostic=QString::fromStdString(e.GetFullMessage());}
   observations.append(QJsonObject{{"case",key},{"provider",provider},{"type",types[k]},{"actual",actual},{"rejected",rejected},{"diagnostic",diagnostic},{"owner_unchanged",before==snapshot(sat)},{"guide_unchanged",guideBefore==snapshot(ref)},{"cache_atomic",!rejected||cache==matrix(p->GetRmatrix())},{"readonly",!p->IsSettable()},{"owner_bound",p->GetRefObject(Gmat::SPACECRAFT,"Sat")==sat},{"frame_bound",p->GetRefObject(Gmat::COORDINATE_SYSTEM,key.toStdString())==frame}});
  }
 }
 QJsonObject proof{{"status","procedure-completed"},{"integration_seconds",integration},{"owner_state",state(sat)},{"guide_state",state(ref)},{"native_stm",matrix(sat->GetRmatrixParameter("OrbitSTM"))},{"guide_stm",matrix(ref->GetRmatrixParameter("OrbitSTM"))},{"observations",observations},{"native_missions",1}};save(output+"/proof.json",QJsonDocument(proof).toJson());std::cout<<"STM owner-dependent frame60reads completed\n";return 0;
}catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;}
