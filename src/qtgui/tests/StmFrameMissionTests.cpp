#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStmParameters.hpp"
#include "CoordinateSystem.hpp"
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
#include <stdexcept>

namespace
{
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void save(const QString &path,const QByteArray &raw){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(raw)==raw.size(),"STM mission evidence write failed");}
QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"STM mission input missing");return f.readAll();}
QJsonArray array(const Rmatrix &m){QJsonArray rows;for(int i=0;i<m.GetNumRows();++i){QJsonArray row;for(int j=0;j<m.GetNumColumns();++j)row.append(m(i,j));rows.append(row);}return rows;}
QJsonArray vector(const Rvector6 &v){QJsonArray a;for(int i=0;i<6;++i)a.append(v[i]);return a;}
bool same(const Rmatrix &a,const Rmatrix &b){if(a.GetNumRows()!=b.GetNumRows()||a.GetNumColumns()!=b.GetNumColumns())return false;for(int i=0;i<a.GetNumRows();++i)for(int j=0;j<a.GetNumColumns();++j)if(a(i,j)!=b(i,j))return false;return true;}
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try{
      require(argc==3,"Supply startup and frozen mission plan");const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");require(!output.isEmpty(),"Supply evidence directory");
      const auto plan=QJsonDocument::fromJson(read(argv[2])).object();const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());auto *engine=Moderator::Instance();require(engine->Initialize(startup.toStdString(),false),"STM mission runtime failed");
      const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");if(!ready.isEmpty()){save(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();while(!QFile::exists(ack)&&timer.elapsed()<10000)QThread::msleep(5);require(QFile::exists(ack),"STM mission images not acknowledged");}
      const double mu=plan["mu"].toDouble(),radius=plan["radius"].toDouble(),referenceRadius=plan["reference_radius"].toDouble();
      QString script=QString("Earth.Mu = %1;\nCreate Spacecraft Sat Ref;\n").arg(mu,0,'g',17);
      const QStringList fields{"X","Y","Z","VX","VY","VZ"},types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"};
      for(const auto &name:{QString("Sat"),QString("Ref")}){
         const double r=name=="Sat"?radius:referenceRadius;const double values[]={r,0,0,0,std::sqrt(mu/r),0};
         script+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545.125';\n";
         for(int i=0;i<6;++i)script+=name+"."+fields[i]+" = "+QString::number(values[i],'g',17)+";\n";
      }
      script+="Create CoordinateSystem ReferenceVNB SelfFrame;\nReferenceVNB.Origin = Earth;\nReferenceVNB.Axes = ObjectReferenced;\nReferenceVNB.Primary = Earth;\nReferenceVNB.Secondary = Ref;\nReferenceVNB.XAxis = V;\nReferenceVNB.ZAxis = N;\nSelfFrame.Origin = Sat;\nSelfFrame.Axes = MJ2000Eq;\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag = None;\nGravity.SRP = Off;\nGravity.RelativisticCorrection = Off;\nCreate Propagator P;\nP.FM = Gravity;\nP.Type = RungeKutta89;\nP.Accuracy = 1e-13;\nP.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\n";
      for(int i=0;i<types.size();++i){const auto report="Matrix"+QString::number(i);script+="Create ReportFile "+report+";\n"+report+".Filename = '"+QDir(output).filePath(types[i]+".txt")+"';\n"+report+".WriteHeaders = false;\n"+report+".Precision = 25;\n";}
      script+="BeginMissionSequence;\n";
      QJsonArray stops=plan["stops"].toArray();if(stops.isEmpty())stops.append(plan["duration_seconds"]);
      double previousStop=0;
      for(const auto &stop:stops){const double endpoint=stop.toDouble();script+="Propagate P(Sat, Ref, 'STM') {Sat.ElapsedSecs = "+QString::number(endpoint-previousStop,'g',17)+", StopTolerance = 1e-6};\n";previousStop=endpoint;}
      for(int i=0;i<types.size();++i)script+="Report Matrix"+QString::number(i)+" Sat.ReferenceVNB."+types[i]+";\n";
      const auto path=QDir(output).filePath("mission.script");save(path,script.toUtf8());require(engine->InterpretScript(path.toStdString()),"External-reference STM mission did not build");
      require(engine->RunMission()==1,"External-reference STM mission did not complete");
      auto *sat=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat"));auto *reference=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));auto *frame=dynamic_cast<CoordinateSystem*>(engine->GetInternalObject("ReferenceVNB"));auto *self=dynamic_cast<CoordinateSystem*>(engine->GetInternalObject("SelfFrame"));require(sat&&reference&&frame&&self,"Executed STM frame objects absent");
      const Rmatrix native=sat->GetRmatrixParameter("OrbitSTM");const auto satEpoch=sat->GetEpochGT(),refEpoch=reference->GetEpochGT();
      QJsonObject proof{{"status","passed"},{"native_missions",1},{"elapsed_seconds",(satEpoch-GmatTime(21545.125)).GetTimeInSec()},{"internal_stm",array(native)},{"sat_state",vector(Rvector6(sat->GetState().GetState()))},{"reference_state",vector(Rvector6(reference->GetState().GetState()))},{"reference_bound_to_executed_object",frame->GetSecondaryObject()==reference}};
      auto check=[&](CoordinateSystem *selected,const QString &name){OrbitStm value("Sat."+selected->GetName()+".OrbitSTM",sat);value.SetSolarSystem(engine->GetSolarSystemInUse());value.SetInternalCoordSystem(sat->GetInternalCoordSystem());value.SetRefObjectName(Gmat::COORDINATE_SYSTEM,selected->GetName());require(value.SetRefObject(selected,Gmat::COORDINATE_SYSTEM,selected->GetName())&&value.Initialize(),"Rejection parameter did not bind");bool rejected=false;QString diagnostic;try{value.EvaluateRmatrix();}catch(BaseException &e){rejected=true;diagnostic=QString::fromStdString(e.GetFullMessage());}const bool unchanged=same(native,sat->GetRmatrixParameter("OrbitSTM"))&&sat->GetEpochGT()==satEpoch;require(rejected&&!diagnostic.isEmpty()&&unchanged,"Undefined STM frame rejection failed or changed state");proof[name]=QJsonObject{{"rejected",rejected},{"diagnostic",diagnostic},{"owner_unchanged",unchanged}};};
      check(self,"self_origin_rejection");
      GmatTime later(refEpoch);later.AddSeconds(1);reference->SetEpochGT(later);check(frame,"noncoeval_rejection");reference->SetEpochGT(refEpoch);
      proof["reference_epoch_restored"]=reference->GetEpochGT()==refEpoch;
      save(QDir(output).filePath("proof.json"),QJsonDocument(proof).toJson());std::cout<<"STM external-reference mission passed\n";return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;
}
