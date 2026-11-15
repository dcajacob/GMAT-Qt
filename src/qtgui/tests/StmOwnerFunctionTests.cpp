#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "Parameter.hpp"
#include "GmatCommand.hpp"
#include "CommandExecutionObserver.hpp"
#include "ObjectManagedFunction.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "Sandbox.hpp"
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
#include <set>

namespace {
void need(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void save(const QString &path,const QByteArray &bytes){QFile f(path);need(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"STM function evidence write failed");}
QByteArray read(const QString &path){QFile f(path);need(f.open(QIODevice::ReadOnly),"STM function fixture missing");return f.readAll();}
QJsonArray matrix(const Rmatrix &m){QJsonArray rows;for(int i=0;i<m.GetNumRows();++i){QJsonArray row;for(int j=0;j<m.GetNumColumns();++j)row.append(m(i,j));rows.append(row);}return rows;}
QJsonArray state(Spacecraft *s){QJsonArray a;for(int i=0;i<6;++i)a.append(s->GetState()[i]);return a;}
QString pointer(const void *p){return QString::number(quintptr(p),16);}
GmatTime epoch(Spacecraft *s){return s->HasPrecisionTime()?s->GetEpochGT():GmatTime(s->GetEpoch());}
QStringList types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"};
class Observer final:public Subscriber,public CommandExecutionObserver {
public:
   Observer(const QString &output,const QString &mode):Subscriber("StmFunctionObserver","StmFunctionObserver"),output(output),mode(mode){}
   ~Observer()override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone()const override{return new Observer(*this);}
   void BeforeExecution(GmatCommand *command)override{
      if(!armed&&command->GetTypeName()=="Propagate"){
         auto *publisher=Publisher::Instance();SetInternalCoordSystem(publisher->GetInternalCoordSystem());
         if(auto *frame=publisher->GetDataCoordSystem())SetDataCoordSystem(frame);
         need(publisher->Subscribe(this),"STM function observer did not subscribe");armed=true;
      }
   }
   bool ReceiveData(const Real *data,Integer count=0)override{
      bool initial=false;if(!Publisher::GetPublishedPropagationObjects(data,count,initial))return true;
      try{
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);need(command,"STM function provider is not a native command");
         auto *function=dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction());if(!function)return true;
         need(function->GetName()=="InspectStm","Unexpected STM sampling function");
         const auto *locals=function->GetFunctionObjectStore(),*globals=function->GetGlobalObjectStore();need(locals&&globals,"STM function live maps missing");
         auto lookup=[&](const std::string &name)->GmatBase*{auto a=locals->find(name);if(a!=locals->end())return a->second;auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};
         auto *craft=dynamic_cast<Spacecraft*>(lookup("craft")),*guide=dynamic_cast<Spacecraft*>(lookup("guide"));
         auto *calls=lookup("Calls");need(craft&&guide&&calls,"STM live function inputs missing");
         const int call=int(calls->GetRealParameter("Value"));const double t=(epoch(craft)-GmatTime(21545.125)).GetTimeInSec();
         const QString phase=initial?"before":std::abs(t-90.)<1e-5?"after":QString();if(phase.isEmpty())return true;
         const auto key=QString::number(call)+phase;if(!seen.insert(key).second)return true;
         const auto frameName=mode=="input"?"axes":"InnerFrame";
         auto *frame=dynamic_cast<CoordinateSystem*>(lookup(frameName));need(frame,"STM function frame missing");
         auto *rootCraft=Moderator::Instance()->GetInternalObject("Sat"),*rootGuide=Moderator::Instance()->GetInternalObject("Ref");
         const bool privateInputs=craft!=rootCraft&&guide!=rootGuide&&craft->IsLocal()&&guide->IsLocal();
         const bool rebound=frame->GetPrimaryObject()==guide&&frame->GetSecondaryObject()==craft;
         QJsonObject sample{{"call",call},{"phase",phase},{"seconds",t},{"guide_seconds",(epoch(guide)-GmatTime(21545.125)).GetTimeInSec()},{"function",QString::fromStdString(function->GetName())},{"craft_pointer",pointer(craft)},{"guide_pointer",pointer(guide)},{"frame_pointer",pointer(frame)},{"frame_primary_pointer",pointer(frame->GetPrimaryObject())},{"root_craft_pointer",pointer(rootCraft)},{"root_guide_pointer",pointer(rootGuide)},{"private_inputs",privateInputs},{"references_rebound",rebound},{"state",state(craft)},{"guide_state",state(guide)},{"guide_stm",matrix(guide->GetRmatrixParameter("OrbitSTM"))},{"native_stm",matrix(craft->GetRmatrixParameter("OrbitSTM"))}};
         samples.append(sample);save(output+"/live-samples.json",QJsonDocument(samples).toJson());
         need(privateInputs&&rebound,"STM function spacecraft/frame references alias the wrong scope");
         QJsonArray reads;
         for(const auto &type:types){const auto name="craft."+QString(frameName)+"."+type;auto *parameter=dynamic_cast<Parameter*>(lookup(name.toStdString()));need(parameter,"STM local native Parameter missing");
            const bool ownerBound=parameter->GetRefObject(Gmat::SPACECRAFT,"craft")==craft;
            const bool frameBound=parameter->GetRefObject(Gmat::COORDINATE_SYSTEM,frameName)==frame;
            const auto value=matrix(parameter->EvaluateRmatrix());need(ownerBound&&frameBound&&!parameter->IsSettable(),"STM native Parameter ownership/readonly metadata differs");
            reads.append(QJsonObject{{"type",type},{"reference",name},{"actual",value},{"owner_bound",ownerBound},{"frame_bound",frameBound},{"readonly",true}});
         }
         sample["reads"]=reads;samples[samples.size()-1]=sample;save(output+"/live-samples.json",QJsonDocument(samples).toJson());
      }catch(BaseException &e){if(error.isEmpty())error=QString::fromStdString(e.GetFullMessage());}
       catch(std::exception &e){if(error.isEmpty())error=e.what();}
      return true;
   }
   bool armed=false;QJsonArray samples;QString error;
protected:
   bool Distribute(Integer)override{return true;}bool Distribute(const Real*,Integer)override{return true;}
private:
   QString output,mode;std::set<QString> seen;
};
}
int main(int argc,char **argv){
   QCoreApplication app(argc,argv);
   try{
      need(argc==3,"Supply STM function startup and frozen plan");const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");need(!output.isEmpty(),"Supply STM function evidence directory");
      const auto plan=QJsonDocument::fromJson(read(argv[2])).object();const auto mode=plan["frame_mode"].toString();need(mode=="input"||mode=="local","Unknown STM function frame mode");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());auto *engine=Moderator::Instance();need(engine->Initialize(startup.toStdString(),false),"STM function runtime failed");
      const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");if(!ready.isEmpty()){save(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer timer;timer.start();while(!QFile::exists(ack)&&timer.elapsed()<10000)QThread::msleep(5);need(QFile::exists(ack),"STM function native images not acknowledged");}
      auto frameCode=[](const QString &name,const QString &guide,const QString &owner){return "Create CoordinateSystem "+name+";\n"+name+".Origin = Earth;\n"+name+".Axes = ObjectReferenced;\n"+name+".Primary = "+guide+";\n"+name+".Secondary = "+owner+";\n"+name+".XAxis = R;\n"+name+".ZAxis = N;\n";};
      auto propagator=[](const QString &name){return "Create ForceModel "+name+"FM;\n"+name+"FM.PrimaryBodies = {};\n"+name+"FM.PointMasses = {Earth};\n"+name+"FM.Drag = None;\n"+name+"FM.SRP = Off;\n"+name+"FM.RelativisticCorrection = Off;\nCreate Propagator "+name+";\n"+name+".FM = "+name+"FM;\n"+name+".Type = RungeKutta89;\n"+name+".Accuracy = 1e-13;\n"+name+".InitialStepSize = 5;\n"+name+".MinStep = 5;\n"+name+".MaxStep = 5;\n";};
      const auto function=QDir(output).filePath("InspectStm.gmf"),outer=QDir(output).filePath("NestedStm.gmf");
      QStringList shared;for(const auto &prefix:{QString("Before"),QString("After")})for(int i=0;i<5;++i)shared<<prefix+QString::number(i);shared<<"Calls";
      const auto selected=mode=="input"?"axes":"InnerFrame";
      QString body="function [mark] = InspectStm(craft, guide, axes)\nCreate Variable mark;\n"+propagator("Inner")+(mode=="local"?frameCode("InnerFrame","guide","craft"):QString())+"BeginMissionSequence;\nGlobal "+shared.join(' ')+";\nCalls = Calls + 1;\n";
      for(int k=0;k<5;++k)body+="Report Before"+QString::number(k)+" craft."+selected+"."+types[k]+";\n";
      body+="Propagate Inner(craft, guide, 'STM') {craft.ElapsedSecs = 30, StopTolerance = 1e-6};\n";
      for(int k=0;k<5;++k)body+="Report After"+QString::number(k)+" craft."+selected+"."+types[k]+";\n";
      body+="mark = craft.EarthMJ2000Eq.X;\n";save(function,body.toUtf8());
      save(outer,("function [mark] = NestedStm(outerCraft, outerGuide, outerAxes)\nCreate Variable mark;\nCreate GmatFunction InspectStm;\nInspectStm.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[mark] = InspectStm(outerCraft, outerGuide, outerAxes);\n").toUtf8());
      const double mu=plan["mu"].toDouble();QString script="Earth.Mu = "+QString::number(mu,'g',17)+";\nCreate Spacecraft Sat Ref;\n";
      const QStringList fields{"X","Y","Z","VX","VY","VZ"};for(const auto &name:{QString("Sat"),QString("Ref")}){
         const double radius=plan[name=="Sat"?"radius":"reference_radius"].toDouble(),values[]={radius,0,0,0,std::sqrt(mu/radius),0};script+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545.125';\n";for(int i=0;i<6;++i)script+=name+"."+fields[i]+" = "+QString::number(values[i],'g',17)+";\n";
      }
      script+=frameCode("Selected","Ref","Sat")+propagator("OuterProp");for(const auto &prefix:{QString("Before"),QString("After"),QString("Root")})for(int i=0;i<5;++i){const auto name=prefix+QString::number(i);script+="Create ReportFile "+name+";\n"+name+".Filename = '"+QDir(output).filePath(name+".txt")+"';\n"+name+".WriteHeaders = false;\n"+name+".Precision = 25;\n";}
      script+="Create GmatFunction InspectStm NestedStm;\nInspectStm.FunctionPath = '"+function+"';\nNestedStm.FunctionPath = '"+outer+"';\nCreate Variable Calls First Second Third Keep;\nCalls = 0;\nKeep = 41;\nBeginMissionSequence;\nGlobal "+shared.join(' ')+";\nPropagate OuterProp(Sat, Ref, 'STM') {Sat.ElapsedSecs = 60, StopTolerance = 1e-6};\n[First] = InspectStm(Sat, Ref, Selected);\n[Second] = InspectStm(Sat, Ref, Selected);\n[Third] = NestedStm(Sat, Ref, Selected);\n";
      for(int k=0;k<5;++k)script+="Report Root"+QString::number(k)+" Sat.Selected."+types[k]+";\n";script+="Keep = Keep + 1;\n";
      const auto path=QDir(output).filePath("mission.script");save(path,script.toUtf8());need(engine->InterpretScript(path.toStdString()),"STM function mission did not build");
      const auto configSat=QString::fromStdString(engine->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::NO_COMMENTS)),configRef=QString::fromStdString(engine->GetConfiguredObject("Ref")->GetGeneratingString(Gmat::NO_COMMENTS));
      Observer observer(output,mode);auto *previous=GmatCommand::SetExecutionObserver(&observer);struct Reset{CommandExecutionObserver *previous;~Reset(){GmatCommand::SetExecutionObserver(previous);}}reset{previous};
      const int code=engine->RunMission();save(output+"/observer-error.txt",observer.error.toUtf8());need(code==1,"STM function mission failed");need(observer.error.isEmpty(),observer.error.toStdString().c_str());need(observer.armed&&observer.samples.size()==6,"STM function callback census differs");
      auto *sat=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat")),*ref=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));need(sat&&ref,"STM caller spacecraft absent");
      QJsonArray marks;for(const auto &name:{"First","Second","Third"})marks.append(engine->GetInternalObject(name)->GetRealParameter("Value"));
      const bool prototypes=configSat==QString::fromStdString(engine->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::NO_COMMENTS))&&configRef==QString::fromStdString(engine->GetConfiguredObject("Ref")->GetGeneratingString(Gmat::NO_COMMENTS));
      QJsonObject proof{{"status","passed"},{"case",plan["key"]},{"frame_mode",mode},{"calls",engine->GetInternalObject("Calls")->GetRealParameter("Value")},{"marks",marks},{"keep",engine->GetInternalObject("Keep")->GetRealParameter("Value")},{"caller_state",state(sat)},{"caller_reference_state",state(ref)},{"caller_seconds",(epoch(sat)-GmatTime(21545.125)).GetTimeInSec()},{"caller_stm",matrix(sat->GetRmatrixParameter("OrbitSTM"))},{"configured_prototypes_unchanged",prototypes},{"samples",observer.samples},{"native_missions",1},{"function_calls",3},{"native_propagations",4}};
      save(output+"/proof.json",QJsonDocument(proof).toJson());std::cout<<"STM function input/frame witnesses completed\n";return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;
}
