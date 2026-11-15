#include "NativeMessageCapture.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "Parameter.hpp"
#include "ObjectManagedFunction.hpp"
#include "CommandExecutionObserver.hpp"
#include "GmatCommand.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "PropSetup.hpp"
#include "PropagationStateManager.hpp"
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

namespace {
void need(bool ok,const char *v){if(!ok)throw std::runtime_error(v);}
void save(const QString &p,const QByteArray &v){QFile f(p);need(f.open(QIODevice::WriteOnly)&&f.write(v)==v.size(),"STM recovery evidence write failed");}
QByteArray read(const QString &p){QFile f(p);need(f.open(QIODevice::ReadOnly),"STM recovery input missing");return f.readAll();}
QJsonArray matrix(const Rmatrix &m){QJsonArray a;for(int i=0;i<m.GetNumRows();++i){QJsonArray row;for(int j=0;j<m.GetNumColumns();++j)row.append(m(i,j));a.append(row);}return a;}
GmatTime epoch(Spacecraft *s){return s->HasPrecisionTime()?s->GetEpochGT():GmatTime(s->GetEpoch());}
QJsonObject snapshot(Spacecraft *s){need(s,"STM recovery spacecraft missing");QJsonArray state;for(int i=0;i<6;++i)state.append(s->GetState()[i]);return QJsonObject{{"state",state},{"stm",matrix(s->GetRmatrixParameter("OrbitSTM"))},{"seconds",(epoch(s)-GmatTime(21545.125)).GetTimeInSec()},{"epoch_mjd",s->GetEpoch()},{"precise",s->HasPrecisionTime()}};}
QStringList types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"};
QJsonArray rootReads(){auto *engine=Moderator::Instance();auto *sat=engine->GetInternalObject("Sat"),*frame=engine->GetInternalObject("Selected");QJsonArray a;for(const auto &t:types){auto *p=dynamic_cast<Parameter*>(engine->GetInternalObject(("Sat.Selected."+t).toStdString()));need(p&&p->GetRefObject(Gmat::SPACECRAFT,"Sat")==sat&&p->GetRefObject(Gmat::COORDINATE_SYSTEM,"Selected")==frame,"STM recovery root Parameter binding differs");a.append(QJsonObject{{"type",t},{"actual",matrix(p->EvaluateRmatrix())},{"readonly",!p->IsSettable()}});}return a;}
class Observer final:public Subscriber,public CommandExecutionObserver {
public:
   Observer(const QString &output,bool reject):Subscriber("StmRecoveryObserver","StmRecoveryObserver"),output(output),reject(reject){}
   ~Observer()override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone()const override{return new Observer(*this);}
   void BeforeExecution(GmatCommand *command)override{
      if(!armed&&command->GetTypeName()=="Propagate"){auto *p=Publisher::Instance();SetInternalCoordSystem(p->GetInternalCoordSystem());if(auto *f=p->GetDataCoordSystem())SetDataCoordSystem(f);need(p->Subscribe(this),"STM recovery observer subscription failed");armed=true;}
      if(command->GetTypeName()=="CallGmatFunction"){
         auto *e=Moderator::Instance();callerBefore=QJsonObject{{"craft",snapshot(dynamic_cast<Spacecraft*>(e->GetInternalObject("Sat")))},{"guide",snapshot(dynamic_cast<Spacecraft*>(e->GetInternalObject("Ref")))},{"reads",rootReads()}};
         save(output+"-caller-before.json",QJsonDocument(callerBefore).toJson());
      }
   }
   bool ReceiveData(const Real *data,Integer count=0)override{
      bool initial=false;if(!Publisher::GetPublishedPropagationObjects(data,count,initial)||inspected)return true;
      try{
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);auto *function=command?dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction()):nullptr;if(!function)return true;
         need(function->GetName()=="RecoverStm","Wrong STM recovery function");const auto *map=function->GetFunctionObjectStore();need(map,"STM recovery local map missing");
         auto lookup=[&](const std::string &name)->GmatBase*{auto it=map->find(name);return it==map->end()?nullptr:it->second;};
         auto *craft=dynamic_cast<Spacecraft*>(lookup("craft")),*guide=dynamic_cast<Spacecraft*>(lookup("guide"));need(craft&&guide,"STM recovery function inputs missing");
         if(initial){
            auto *setup=dynamic_cast<PropSetup*>(command->GetClone(0));need(setup&&setup->GetPropStateManager(),"STM diagnostic propagation state manager missing");auto *manager=setup->GetPropStateManager();
            QJsonArray entries;const auto *map=manager->GetStateMap();for(int i=0;i<int(map->size());++i){const auto *item=(*map)[i];entries.append(QJsonObject{{"index",i},{"object",QString::fromStdString(item->objectName)},{"full_name",QString::fromStdString(item->objectFullName)},{"associate",QString::fromStdString(item->associateName)},{"associate_index",manager->GetState()->GetAssociateIndex(i)},{"element",QString::fromStdString(item->elementName)},{"subelement",item->subelement}});}
            save(output+"-initial-diagnostic.json",QJsonDocument(QJsonObject{{"craft",snapshot(craft)},{"guide",snapshot(guide)},{"craft_full_name",QString::fromStdString(craft->GetFullName())},{"guide_full_name",QString::fromStdString(guide->GetFullName())},{"state_map",entries}}).toJson());return true;
         }
         if(std::abs((epoch(craft)-GmatTime(21545.125)).GetTimeInSec()-90.)>1e-5)return true;
         inspected=true;auto *frame=dynamic_cast<CoordinateSystem*>(lookup("CheckFrame"));need(frame,"STM recovery frame missing");
         const bool privateInputs=craft!=Moderator::Instance()->GetInternalObject("Sat")&&guide!=Moderator::Instance()->GetInternalObject("Ref")&&craft->IsLocal()&&guide->IsLocal();
         local=QJsonObject{{"craft",snapshot(craft)},{"guide",snapshot(guide)},{"private_inputs",privateInputs},{"self_origin",frame->GetOrigin()==craft}};QJsonArray rows;
         for(const auto &type:types){auto *p=dynamic_cast<Parameter*>(lookup(("craft.CheckFrame."+type).toStdString()));need(p,"STM recovery native Parameter missing");const auto before=snapshot(craft),beforeGuide=snapshot(guide);const auto cache=matrix(p->GetRmatrix());bool rejected=false;QString diagnostic;QJsonArray actual;
            try{actual=matrix(p->EvaluateRmatrix());}catch(BaseException &e){rejected=true;diagnostic=QString::fromStdString(e.GetFullMessage());}
            rows.append(QJsonObject{{"type",type},{"rejected",rejected},{"diagnostic",diagnostic},{"actual",actual},{"owner_unchanged",before==snapshot(craft)},{"guide_unchanged",beforeGuide==snapshot(guide)},{"cache_unchanged",cache==matrix(p->GetRmatrix())},{"owner_bound",p->GetRefObject(Gmat::SPACECRAFT,"craft")==craft},{"frame_bound",p->GetRefObject(Gmat::COORDINATE_SYSTEM,"CheckFrame")==frame},{"readonly",!p->IsSettable()}});
         }local["reads"]=rows;save(output+"-local.json",QJsonDocument(local).toJson());need(privateInputs,"STM recovery inputs alias caller");
      }catch(BaseException &e){if(error.isEmpty())error=QString::fromStdString(e.GetFullMessage());}
       catch(std::exception &e){if(error.isEmpty())error=e.what();}
      return true;
   }
   bool armed=false,inspected=false;QJsonObject callerBefore,local;QString error;
protected:bool Distribute(Integer)override{return true;}bool Distribute(const Real*,Integer)override{return true;}
private:QString output;bool reject;
};
}
int main(int argc,char **argv){QCoreApplication app(argc,argv);try{
   need(argc==3,"Supply STM recovery startup and frozen plan");const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");need(!output.isEmpty(),"Supply STM recovery evidence directory");const auto plan=QJsonDocument::fromJson(read(argv[2])).object();
   NativeMessageCapture messages;const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());auto *engine=Moderator::Instance();need(engine->Initialize(startup.toStdString(),false),"STM recovery runtime failed");
   const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");if(!ready.isEmpty()){save(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer t;t.start();while(!QFile::exists(ack)&&t.elapsed()<10000)QThread::msleep(5);need(QFile::exists(ack),"STM recovery native images not acknowledged");}
   auto prop=[](const QString &name){return "Create ForceModel "+name+"FM;\n"+name+"FM.PrimaryBodies = {};\n"+name+"FM.PointMasses = {Earth};\n"+name+"FM.Drag = None;\n"+name+"FM.SRP = Off;\n"+name+"FM.RelativisticCorrection = Off;\nCreate Propagator "+name+";\n"+name+".FM = "+name+"FM;\n"+name+".Type = RungeKutta89;\n"+name+".Accuracy = 1e-13;\n"+name+".InitialStepSize = 5;\n"+name+".MinStep = 5;\n"+name+".MaxStep = 5;\n";};
   const auto function=QDir(output).filePath("RecoverStm.gmf"),outer=QDir(output).filePath("NestedRecovery.gmf");save(outer,("function [mark] = NestedRecovery(outerCraft, outerGuide, outerAxes)\nCreate Variable mark;\nCreate GmatFunction RecoverStm;\nRecoverStm.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[mark] = RecoverStm(outerCraft, outerGuide, outerAxes);\n").toUtf8());
   QStringList globals;for(int k=0;k<5;++k)globals<<"Function"+QString::number(k);
   QJsonArray phases;QByteArray failingBody;
   for(bool rejected:{true,false}){
      const auto tag=rejected?"failure":"recovery";
      QString body="function [mark] = RecoverStm(craft, guide, axes)\nCreate Variable mark;\nCreate CoordinateSystem CheckFrame;\nCheckFrame.Origin = "+QString(rejected?"craft":"Earth")+";\nCheckFrame.Axes = MJ2000Eq;\n"+prop("Inner")+"BeginMissionSequence;\nGlobal "+globals.join(' ')+";\nPropagate Inner(craft, guide, 'STM') {craft.ElapsedSecs = 30, StopTolerance = 1e-6};\n";
      for(int k=0;k<5;++k)body+="Report Function"+QString::number(k)+" craft.CheckFrame."+types[k]+";\n";body+="mark = craft.EarthMJ2000Eq.X;\n";
      if(rejected)failingBody=body.toUtf8();else need(body.toUtf8()==QByteArray(failingBody).replace("CheckFrame.Origin = craft;","CheckFrame.Origin = Earth;"),"STM recovery changed more than the invalid frame origin");save(function,body.toUtf8());save(output+"/"+tag+".gmf",body.toUtf8());
      const double mu=plan["mu"].toDouble();QString script="Earth.Mu = "+QString::number(mu,'g',17)+";\nCreate Spacecraft Sat Ref;\n";const QStringList fields{"X","Y","Z","VX","VY","VZ"};
      for(const auto &name:{QString("Sat"),QString("Ref")}){const double radius=plan[name=="Sat"?"radius":"reference_radius"].toDouble(),values[]={radius,0,0,0,std::sqrt(mu/radius),0};script+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545.125';\n";for(int i=0;i<6;++i)script+=name+"."+fields[i]+" = "+QString::number(values[i],'g',17)+";\n";}
      script+="Create CoordinateSystem Selected;\nSelected.Origin = Earth;\nSelected.Axes = ObjectReferenced;\nSelected.Primary = Earth;\nSelected.Secondary = Ref;\nSelected.XAxis = V;\nSelected.ZAxis = N;\n"+prop("Outer");
      for(const auto &prefix:{QString("Function"),QString("Root")})for(int k=0;k<5;++k){const auto name=prefix+QString::number(k);script+="Create ReportFile "+name+";\n"+name+".Filename = '"+QDir(output).filePath(QString(tag)+"-"+name+".txt")+"';\n"+name+".WriteHeaders = false;\n"+name+".Precision = 25;\n";}
      script+="Create GmatFunction NestedRecovery;\nNestedRecovery.FunctionPath = '"+outer+"';\nCreate Variable Result Keep;\nResult = -1;\nKeep = 41;\nBeginMissionSequence;\nGlobal "+globals.join(' ')+";\nPropagate Outer(Sat, Ref, 'STM') {Sat.ElapsedSecs = 60, StopTolerance = 1e-6};\n[Result] = NestedRecovery(Sat, Ref, Selected);\n";for(int k=0;k<5;++k)script+="Report Root"+QString::number(k)+" Sat.Selected."+types[k]+";\n";script+="Keep = Keep + 1;\n";
      const auto path=QDir(output).filePath(QString(tag)+".script");save(path,script.toUtf8());need(engine->InterpretScript(path.toStdString()),"STM recovery mission did not build");const auto prototype=QString::fromStdString(engine->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::NO_COMMENTS));
      Observer observer(output+"/"+tag,rejected);auto *previous=GmatCommand::SetExecutionObserver(&observer);messages.text.clear();const int code=engine->RunMission();GmatCommand::SetExecutionObserver(previous);save(output+"/"+tag+"-messages.txt",QByteArray::fromStdString(messages.text));save(output+"/"+tag+"-observer-error.txt",observer.error.toUtf8());
      need(observer.armed&&observer.inspected&&observer.error.isEmpty(),observer.error.isEmpty()?"STM recovery observer did not complete":observer.error.toStdString().c_str());
      auto *sat=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat")),*ref=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));QJsonObject after{{"craft",snapshot(sat)},{"guide",snapshot(ref)},{"reads",rootReads()}};
      const bool unchanged=after==observer.callerBefore;QJsonObject phase{{"phase",tag},{"run_status",code},{"caller_before",observer.callerBefore},{"caller_after",after},{"caller_unchanged",unchanged},{"local",observer.local},{"result",engine->GetInternalObject("Result")->GetRealParameter("Value")},{"keep",engine->GetInternalObject("Keep")->GetRealParameter("Value")},{"prototype_unchanged",prototype==QString::fromStdString(engine->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::NO_COMMENTS))}};phases.append(phase);save(output+"/phases.json",QJsonDocument(phases).toJson());
      need(code==(rejected?plan["expected_failure_status"].toInt():1),"STM recovery native error/success status differs");need(unchanged,"STM failure/success function changed caller state, STM or selected-frame value");
   }
   QJsonObject proof{{"status","passed"},{"case",plan["key"]},{"phases",phases},{"native_missions",2},{"nested_calls",2},{"native_worker_source_writes",0}};save(output+"/proof.json",QJsonDocument(proof).toJson());std::cout<<"STM function rejection and recovery witnesses completed\n";return 0;
}catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;}
