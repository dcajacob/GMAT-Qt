#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStmParameters.hpp"
#include "AxisSystem.hpp"
#include "ICRFAxes.hpp"
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
#include <memory>
#include <stdexcept>

namespace
{
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly), "STM fixture missing"); return f.readAll(); }
void save(const QString &path, const QByteArray &raw)
{ QFile f(path); require(f.open(QIODevice::WriteOnly) && f.write(raw) == raw.size(), "STM evidence write failed"); }
QJsonArray array(const Rmatrix &matrix)
{ QJsonArray result; for(int i=0;i<matrix.GetNumRows();++i) { QJsonArray row; for(int j=0;j<matrix.GetNumColumns();++j) row.append(matrix(i,j)); result.append(row); } return result; }
bool same(const Rmatrix &a, const Rmatrix &b)
{ if(a.GetNumRows()!=b.GetNumRows()||a.GetNumColumns()!=b.GetNumColumns()) return false; for(int i=0;i<a.GetNumRows();++i) for(int j=0;j<a.GetNumColumns();++j) if(a(i,j)!=b(i,j)) return false; return true; }
class SpinAxes : public AxisSystem
{
public:
   double theta, tilt, rate;
   SpinAxes(double z,double x,double w,const std::string &base):AxisSystem("StmPrescribedSpin"),theta(z),tilt(x),rate(w) { baseSystem=base; }
   GmatBase *Clone() const override { return new SpinAxes(*this); }
   void compute(const GmatTime &epoch)
   {
      const double angle=theta+rate*(epoch-GmatTime(21545.000000397937)).GetTimeInSec();
      const double c=std::cos(angle),s=std::sin(angle),C=std::cos(tilt),S=std::sin(tilt);
      rotMatrix.Set(c,-s*C,s*S,s,c*C,-c*S,0,S,C);
      rotDotMatrix.Set(-rate*s,-rate*c*C,rate*c*S,rate*c,-rate*s*C,rate*s*S,0,0,0);
      rotData=rotMatrix.GetDataVector(); rotDotData=rotDotMatrix.GetDataVector();
   }
   void CalculateRotationMatrix(const A1Mjd &epoch,bool) override { compute(GmatTime(epoch.Get())); }
   void CalculateRotationMatrix(const GmatTime &epoch,bool) override { compute(epoch); }
};
std::unique_ptr<Parameter> parameter(int type, Spacecraft *owner, CoordinateSystem *frame, Moderator *engine)
{
   const auto prefix=owner->GetName()+"."+frame->GetName()+".";
   Parameter *value=nullptr;
   switch(type) {
   case 0:value=new OrbitStm(prefix+"OrbitSTM",owner);break;
   case 1:value=new OrbitStmA(prefix+"OrbitSTMA",owner);break;
   case 2:value=new OrbitStmB(prefix+"OrbitSTMB",owner);break;
   case 3:value=new OrbitStmC(prefix+"OrbitSTMC",owner);break;
   case 4:value=new OrbitStmD(prefix+"OrbitSTMD",owner);break;
   }
   std::unique_ptr<Parameter> result(value);
   result->SetSolarSystem(engine->GetSolarSystemInUse());
   result->SetInternalCoordSystem(owner->GetInternalCoordSystem());
   result->SetRefObjectName(Gmat::COORDINATE_SYSTEM,frame->GetName());
   require(result->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,frame->GetName()),"Selected STM frame did not bind");
   require(result->Initialize(),"Selected STM parameter did not initialize");
   return result;
}
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      require(argc==3,"Supply startup and frozen matrix cases");
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); require(!output.isEmpty(),"Supply evidence directory");
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object(); const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath()); auto *engine=Moderator::Instance();
      require(engine->Initialize(startup.toStdString(),false),"STM frame runtime failed");
      const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");
      if(!ready.isEmpty()) { save(ready,QByteArray::number(QCoreApplication::applicationPid())); QElapsedTimer t;t.start();while(!QFile::exists(ack)&&t.elapsed()<10000)QThread::msleep(5);require(QFile::exists(ack),"STM frame images not acknowledged"); }
      const auto script=QDir(output).filePath("mission.script");
      save(script,"Create Spacecraft Sat;\nSat.CoordinateSystem=EarthMJ2000Eq;\nCreate CoordinateSystem Selected InputFrame;\nSelected.Origin=Earth;\nSelected.Axes=MJ2000Eq;\nInputFrame.Origin=Earth;\nInputFrame.Axes=MJ2000Eq;\nBeginMissionSequence;\n");
      require(engine->InterpretScript(script.toStdString()),"STM frame script failed");
      auto *sat=dynamic_cast<Spacecraft*>(engine->GetConfiguredObject("Sat"));
      auto *frame=dynamic_cast<CoordinateSystem*>(engine->GetConfiguredObject("Selected")); auto *input=dynamic_cast<CoordinateSystem*>(engine->GetConfiguredObject("InputFrame")); require(sat&&frame&&input,"STM owner/frames absent");
      const QStringList types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"}; QJsonArray observations; int failed=0;
      for(const auto &entry:fixture["cases"].toArray()) {
         const auto spec=entry.toObject(); const auto key=spec["key"].toString();
         auto configure=[](CoordinateSystem *target,const QJsonObject &axis) {
            std::unique_ptr<AxisSystem> value;
            if(axis["kind"].toString()=="ICRF") value=std::make_unique<ICRFAxes>();
            else value=std::make_unique<SpinAxes>(axis["theta"].toDouble(),axis["tilt"].toDouble(),axis["rate"].toDouble(),axis["base"].toString().toStdString());
            require(target->SetRefObject(value.get(),Gmat::AXIS_SYSTEM,value->GetName())&&target->Initialize(),"STM base frame failed");
         };
         configure(input,spec["input_axes"].toObject()); configure(frame,spec["output_axes"].toObject());
         sat->SetInternalCoordSystem(input);
         const bool precise=spec["precise"].toBool();sat->SetPrecisionTimeFlag(precise);
         auto stamp=[&](bool final) {
            if(precise) {GmatTime t(spec[final?"final_mjd":"initial_mjd"].toDouble());sat->SetEpochGT(t);}
            else sat->SetEpochGT(GmatTime(spec[final?"final_mjd":"initial_mjd"].toDouble()));
         };
         stamp(false);sat->SetState(Rvector6(7000,0,0,0,7.546053290107541,0));
         if(spec["reset"].toString()=="action") sat->TakeAction("ResetSTM");
         else if(spec["reset"].toString()!="fresh") sat->SetSTMToIdentityMatrix();
         Rmatrix66 raw(false); const auto rows=spec["native_stm"].toArray();
         for(int i=0;i<6;++i)for(int j=0;j<6;++j)raw(i,j)=rows[i].toArray()[j].toDouble();
         sat->SetRmatrixParameter("OrbitSTM",raw);stamp(true);
         std::unique_ptr<Spacecraft> clone(dynamic_cast<Spacecraft*>(sat->Clone()));
         Spacecraft copied("Copied");copied=*sat;require(bool(clone),"Native STM owner clone failed");
         for(const auto &provider:std::vector<std::pair<QString,Spacecraft*>>{{"original",sat},{"clone",clone.get()},{"copy",&copied}}) {
            for(int type=0;type<5;++type) {
               QJsonObject observation{{"case",key},{"provider",provider.first},{"type",types[type]},{"passed",false}};
               try {
                  auto value=parameter(type,provider.second,frame,engine);const Rmatrix before=provider.second->GetRmatrixParameter("OrbitSTM");
                  const Rmatrix actual=value->EvaluateRmatrix();const auto expected=spec["expected_stm"].toArray();const int size=type==0?6:3;
                  const int row=type==0?0:((type-1)/2)*3,col=type==0?0:((type-1)%2)*3;
                  double error=0;bool ok=actual.GetNumRows()==size&&actual.GetNumColumns()==size;
                  if(ok)for(int i=0;i<size;++i)for(int j=0;j<size;++j) {double e=std::abs(actual(i,j)-expected[i+row].toArray()[j+col].toDouble());error=std::max(error,e);ok=ok&&std::isfinite(actual(i,j))&&e<=fixture["matrix_tolerance"].toDouble();}
                  const bool unchanged=same(before,provider.second->GetRmatrixParameter("OrbitSTM"));ok=ok&&unchanged&&!value->IsSettable();
                  observation["actual"]=array(actual);observation["max_error"]=error;observation["owner_unchanged"]=unchanged;observation["passed"]=ok;
               }catch(BaseException &e) {observation["error"]=QString::fromStdString(e.GetFullMessage());}
               failed+=!observation["passed"].toBool();observations.append(observation);
            }
         }
      }
      require(observations.size()==75,"STM frame witness census differs");
      save(QDir(output).filePath("proof.json"),QJsonDocument(QJsonObject{{"status",failed?"failed":"passed"},{"observations",observations},{"failed",failed},{"native_missions",0},{"scope",fixture["scope"]}}).toJson());
      std::cout<<"STM base alignment75reads failed="<<failed<<'\n';return failed?1:0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;
}
