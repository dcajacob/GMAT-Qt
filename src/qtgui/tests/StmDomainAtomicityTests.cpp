// Supplementary domain checks: the public matrix Parameters must fail atomically.
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStmParameters.hpp"
#include "OrbitStmContext.hpp"
#include "AxisSystem.hpp"
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
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
void need(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
QByteArray read(const QString &path) { QFile f(path); need(f.open(QIODevice::ReadOnly), "Missing domain plan"); return f.readAll(); }
void save(const QString &path, const QByteArray &data) { QFile f(path); need(f.open(QIODevice::WriteOnly) && f.write(data)==data.size(), "Cannot save domain proof"); }
QString bits(double value) { return QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(&value),sizeof value).toHex()); }
QJsonArray encoded(const Rmatrix &value) {
   QJsonArray result;
   for (int i=0;i<value.GetNumRows();++i) { QJsonArray row; for(int j=0;j<value.GetNumColumns();++j) row.append(bits(value(i,j))); result.append(row); }
   return result;
}
QJsonArray numeric(const Rmatrix &value) {
   QJsonArray result;
   for (int i=0;i<value.GetNumRows();++i) { QJsonArray row; for(int j=0;j<value.GetNumColumns();++j) row.append(value(i,j)); result.append(row); }
   return result;
}
QJsonObject sample(const std::shared_ptr<OrbitStmContext::Sample> &value) {
   if(!value) return {};
   QJsonArray state, acceleration;
   for(int i=0;i<6;++i) state.append(bits(value->state[i]));
   for(int i=0;i<3;++i) acceleration.append(bits(value->acceleration[i]));
   return {{"generation",QString::number(value->generation)}, {"state",state},
      {"acceleration_available",value->accelerationAvailable}, {"acceleration",acceleration},
      {"acceleration_origin",QString::number(reinterpret_cast<quintptr>(value->accelerationOrigin))},
      {"gradient",value->accelerationGradient ? encoded(*value->accelerationGradient) : QJsonArray()}};
}
QJsonObject history(Spacecraft *object) {
   const auto value=OrbitStmContext::Get(object); QJsonObject result{{"advanced",value.advanced},{"present",bool(value.reference)}};
   if(value.reference) {
      result["epoch"]=QString::fromStdString(value.reference->epoch.ToString());
      result["precise"]=value.reference->precise; result["self"]=sample(value.reference->self);
      QJsonObject world;
      for(const auto &entry:value.reference->world) world[QString::number(reinterpret_cast<quintptr>(entry.first))]=sample(entry.second);
      result["world"]=world;
   }
   return result;
}
QJsonObject snapshot(Spacecraft *object) {
   QJsonArray state; for(int i=0;i<object->GetState().GetSize();++i) state.append(bits(object->GetState()[i]));
   return {{"epoch",QString::fromStdString(object->GetEpochGT().ToString())}, {"precise",object->HasPrecisionTime()},
      {"state",state},{"stm",encoded(object->GetRmatrixParameter("OrbitSTM"))},{"history",history(object)}};
}
QJsonObject frameSnapshot(CoordinateSystem *frame) {
   return {{"rotation",encoded(frame->GetLastRotationMatrix())}, {"rate",encoded(frame->GetLastRotationDotMatrix())},
      {"epoch",bits(frame->GetEpoch().Get())}};
}
class DomainAxes : public AxisSystem {
public:
   std::string failure;
   DomainAxes(const std::string &mode="") : AxisSystem("StmDomainSpin"),failure(mode) { baseSystem="FK5"; }
   GmatBase *Clone() const override { return new DomainAxes(*this); }
   void compute(const GmatTime &epoch) {
      const double seconds=(epoch-GmatTime(21545.125)).GetTimeInSec(), w=0.001;
      const double a=0.37+w*seconds, c=std::cos(a),s=std::sin(a),C=std::cos(0.23),S=std::sin(0.23);
      rotMatrix.Set(c,-s*C,s*S,s,c*C,-c*S,0,S,C);
      rotDotMatrix.Set(-w*s,-w*c*C,w*c*S,w*c,-w*s*C,w*s*S,0,0,0);
      if(failure=="axis_nan") rotMatrix(0,0)=std::numeric_limits<double>::quiet_NaN();
      if(failure=="axis_inf_rate") rotDotMatrix(2,1)=std::numeric_limits<double>::infinity();
      if(failure=="singular_initial" && std::abs(seconds)<0.5) rotMatrix=Rmatrix33(false);
      rotData=rotMatrix.GetDataVector();rotDotData=rotDotMatrix.GetDataVector();
   }
   void CalculateRotationMatrix(const A1Mjd &epoch,bool) override {compute(GmatTime(epoch.Get()));}
   void CalculateRotationMatrix(const GmatTime &epoch,bool) override {compute(epoch);}
};
void axes(CoordinateSystem *frame,const std::string &failure="") {
   DomainAxes value(failure); need(frame->SetRefObject(&value,Gmat::AXIS_SYSTEM,value.GetName())&&frame->Initialize(),"Cannot set domain axes");
}
std::unique_ptr<Parameter> parameter(int type,Spacecraft *owner,CoordinateSystem *frame,Moderator *engine) {
   const auto name=owner->GetName()+"."+frame->GetName()+".";
   Parameter *value=nullptr;
   switch(type) {
      case 0:value=new OrbitStm(name+"OrbitSTM",owner);break;
      case 1:value=new OrbitStmA(name+"OrbitSTMA",owner);break;
      case 2:value=new OrbitStmB(name+"OrbitSTMB",owner);break;
      case 3:value=new OrbitStmC(name+"OrbitSTMC",owner);break;
      case 4:value=new OrbitStmD(name+"OrbitSTMD",owner);break;
   }
   std::unique_ptr<Parameter> result(value); result->SetSolarSystem(engine->GetSolarSystemInUse());
   result->SetInternalCoordSystem(owner->GetInternalCoordSystem());
   result->SetRefObjectName(Gmat::COORDINATE_SYSTEM,frame->GetName());
   need(result->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,frame->GetName())&&result->Initialize(),"Cannot bind matrix Parameter");
   return result;
}
Rmatrix nativeMatrix(const QJsonArray &values) {
   Rmatrix result(6,6);for(int i=0;i<6;++i)for(int j=0;j<6;++j)result(i,j)=values[i].toArray()[j].toDouble();return result;
}
}
int main(int argc,char **argv) {
   QCoreApplication app(argc,argv);
   try {
      need(argc==3,"Supply startup and frozen domain plan"); const auto plan=QJsonDocument::fromJson(read(argv[2])).object();
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");need(!output.isEmpty(),"Supply proof directory");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());auto *engine=Moderator::Instance();
      need(engine->Initialize(startup.toStdString(),false),"Domain runtime initialization failed");
      const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");
      if(!ready.isEmpty()) {save(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer t;t.start();while(!QFile::exists(ack)&&t.elapsed()<10000)QThread::msleep(5);need(QFile::exists(ack),"Domain image receipt missing");}
      const auto script=QDir(output).filePath("mission.script");
      save(script,"Create Spacecraft Sat Ref;\nSat.CoordinateSystem=EarthMJ2000Eq;\nRef.CoordinateSystem=EarthMJ2000Eq;\nCreate CoordinateSystem Selected GuideFrame;\nSelected.Origin=Earth;\nSelected.Axes=MJ2000Eq;\nGuideFrame.Origin=Ref;\nGuideFrame.Axes=MJ2000Eq;\nBeginMissionSequence;\n");
      need(engine->InterpretScript(script.toStdString()),"Domain script failed");
      auto *sat=dynamic_cast<Spacecraft*>(engine->GetConfiguredObject("Sat")),*ref=dynamic_cast<Spacecraft*>(engine->GetConfiguredObject("Ref"));
      auto *selected=dynamic_cast<CoordinateSystem*>(engine->GetConfiguredObject("Selected")),*guide=dynamic_cast<CoordinateSystem*>(engine->GetConfiguredObject("GuideFrame"));
      need(sat&&ref&&selected&&guide,"Missing domain objects");
      QJsonArray observations;const QStringList types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"};
      const auto base=nativeMatrix(plan["native_stm"].toArray());
      for(const auto &entry:plan["cases"].toArray()) {
         const auto spec=entry.toObject();const auto key=spec["key"].toString();const auto mode=spec["mode"].toString();
         for(int type=0;type<5;++type) {
            save(output+"/cursor.json",QJsonDocument(QJsonObject{{"case",key},{"type",types[type]},{"completed",observations.size()}}).toJson());
            axes(selected);axes(guide);sat->SetPrecisionTimeFlag(true);ref->SetPrecisionTimeFlag(true);
            sat->SetIntegerParameter("FullSTMRowCount",6);
            sat->SetState(Rvector6(7000,100,200,-0.2,7.4,0.3));ref->SetState(Rvector6(8100,300,100,-0.1,6.8,0.2));
            sat->SetEpochGT(GmatTime(21545.125));ref->SetEpochGT(GmatTime(21545.125));
            ref->SetSTMToIdentityMatrix();sat->SetSTMToIdentityMatrix();sat->SetRmatrixParameter("OrbitSTM",base);
            GmatTime final(21545.125);final.AddSeconds(45);sat->SetEpochGT(final);ref->SetEpochGT(final);
            auto value=parameter(type,sat,selected,engine);value->EvaluateRmatrix();
            CoordinateSystem *active=selected;
            if(mode.startsWith("guide_")) {active=guide;value=parameter(type,sat,guide,engine);}
            if(mode=="matrix_nan"||mode=="matrix_inf"||mode=="matrix_neg_inf") {
               auto matrix=base;matrix(spec["row"].toInt(),spec["column"].toInt())=mode=="matrix_nan"?std::numeric_limits<double>::quiet_NaN():mode=="matrix_inf"?std::numeric_limits<double>::infinity():-std::numeric_limits<double>::infinity();sat->SetRmatrixParameter("OrbitSTM",matrix);
            } else if(mode=="short_rows"||mode=="short_columns") {
               // A native writer's malformed buffer is injected through its
               // mutable storage hook; the user-facing setter keeps its shape.
               auto *storage=sat->GetParameterSTM(sat->GetParameterID("X"));
               storage->SetSize(mode=="short_rows"?5:6,mode=="short_columns"?5:6);
            } else if(mode=="axis_nan"||mode=="axis_inf_rate"||mode=="singular_initial") axes(selected,mode.toStdString());
            else if(mode=="missing_history") OrbitStmContext::Commit(sat,{});
            else if(mode=="fresh_invalid_axes"||mode=="fresh_valid_axes"||mode=="nonfinite_fresh_state") {
               sat->SetSTMToIdentityMatrix();OrbitStmContext::Commit(sat,{});
               if(mode=="fresh_invalid_axes") axes(selected,"axis_nan");
               else if(mode=="nonfinite_fresh_state") sat->GetState()[0]=std::numeric_limits<double>::quiet_NaN();
            } else if(mode=="guide_not_coeval") {GmatTime late=final;late.AddSeconds(1);ref->SetEpochGT(late);}
            else if(mode=="guide_missing_history"||mode=="guide_stale_generation") {
               auto current=OrbitStmContext::Get(sat);auto copy=std::make_shared<OrbitStmContext::Reference>(*current.reference);
               if(mode=="guide_missing_history") {copy->world.erase(ref);OrbitStmContext::Commit(ref,{});}
               else {auto old=std::make_shared<OrbitStmContext::Sample>(*copy->world.at(ref));old->generation=0;copy->world[ref]=old;OrbitStmContext::Commit(ref,{});}
               current.reference=copy;OrbitStmContext::Commit(sat,current);
            } else if(mode=="extended_finite"||mode=="extended_nonfinite_tail") {
               Rmatrix matrix(9,9);for(int i=0;i<9;++i)for(int j=0;j<9;++j)matrix(i,j)=i<6&&j<6?base(i,j):100.0+i*9+j;
               if(mode=="extended_nonfinite_tail") matrix(8,8)=std::numeric_limits<double>::quiet_NaN();
               sat->SetIntegerParameter("FullSTMRowCount",9);sat->SetRmatrixParameter("OrbitSTM",matrix);
            } else if(mode=="singular_phi") sat->SetRmatrixParameter("OrbitSTM",Rmatrix(6,6));
            const auto before=snapshot(sat),guideBefore=snapshot(ref),frameBefore=frameSnapshot(active);const auto cacheBefore=encoded(value->GetRmatrix());
            bool rejected=false;QString diagnostic;QJsonArray actual;
            try {actual=numeric(value->EvaluateRmatrix());}catch(BaseException &e){rejected=true;diagnostic=QString::fromStdString(e.GetFullMessage());}
            observations.append(QJsonObject{{"case",key},{"type",types[type]},{"rejected",rejected},{"diagnostic",diagnostic},{"actual",actual},
               {"owner_before",before},{"owner_after",snapshot(sat)},{"guide_before",guideBefore},{"guide_after",snapshot(ref)},
               {"frame_before",frameBefore},{"frame_after",frameSnapshot(active)},{"cache_before",cacheBefore},{"cache_after",encoded(value->GetRmatrix())},{"readonly",!value->IsSettable()}});
            save(output+"/partial-proof.json",QJsonDocument(QJsonObject{{"observations",observations}}).toJson());
         }
      }
      save(output+"/proof.json",QJsonDocument(QJsonObject{{"status","procedure-completed"},{"observations",observations},{"native_missions",0}}).toJson());
      std::cout<<"STM domain observations "<<observations.size()<<'\n';return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;
}
