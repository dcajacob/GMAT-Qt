#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "CoordinateSystem.hpp"
#include "Parameter.hpp"
#include "GmatCommand.hpp"
#include "CommandExecutionObserver.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
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
void need(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void save(const QString &path,const QByteArray &bytes){QFile f(path);need(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"STM solver evidence write failed");}
QByteArray read(const QString &path){QFile f(path);need(f.open(QIODevice::ReadOnly),"STM solver fixture missing");return f.readAll();}
QJsonArray matrix(const Rmatrix &m){QJsonArray a;for(int i=0;i<m.GetNumRows();++i){QJsonArray row;for(int j=0;j<m.GetNumColumns();++j)row.append(m(i,j));a.append(row);}return a;}
QJsonArray state(Spacecraft *s){QJsonArray a;for(int i=0;i<6;++i)a.append(s->GetState()[i]);return a;}
GmatTime epoch(Spacecraft *s){return s->HasPrecisionTime()?s->GetEpochGT():GmatTime(s->GetEpoch());}
QString number(double value){return QString::number(value,'g',17);}
QStringList types{"OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"};
class Observer final:public CommandExecutionObserver {
public:
   explicit Observer(const QString &output):output(output){}
   void BeforeExecution(GmatCommand *command)override{
      const auto source=QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS));
      QString phase;if(command->GetTypeName()=="Propagate"&&source.contains("'SolverArc'"))phase="before";
      if(command->GetTypeName()=="Report"&&source.contains("'TrialFull'"))phase="after";
      if(phase.isEmpty())return;
      try{
         auto *engine=Moderator::Instance();auto *sat=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat")),*ref=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));
         auto *frame=dynamic_cast<CoordinateSystem*>(engine->GetInternalObject("Selected"));auto *control=engine->GetInternalObject("Phase");need(sat&&ref&&frame&&control,"STM solver live graph missing");
         QJsonObject sample{{"phase",phase},{"angle",control->GetRealParameter("Value")},{"seconds",(epoch(sat)-GmatTime(21545.125)).GetTimeInSec()},{"reference_seconds",(epoch(ref)-GmatTime(21545.125)).GetTimeInSec()},{"state",state(sat)},{"reference_state",state(ref)},{"native_stm",matrix(sat->GetRmatrixParameter("OrbitSTM"))},{"frame_rebound",frame->GetSecondaryObject()==ref},{"command",source}};
         samples.append(sample);save(output+"/trial-samples.json",QJsonDocument(samples).toJson());
         if(phase=="after"){
            QJsonArray reads;for(const auto &type:types){const auto name="Sat.Selected."+type;auto *parameter=dynamic_cast<Parameter*>(engine->GetInternalObject(name.toStdString()));need(parameter,"STM solver native Parameter missing");
               const bool owner=parameter->GetRefObject(Gmat::SPACECRAFT,"Sat")==sat,coordinate=parameter->GetRefObject(Gmat::COORDINATE_SYSTEM,"Selected")==frame;
               const auto actual=matrix(parameter->EvaluateRmatrix());need(owner&&coordinate&&!parameter->IsSettable(),"STM solver Parameter binding or readonly metadata differs");
               reads.append(QJsonObject{{"type",type},{"actual",actual},{"owner_bound",owner},{"frame_bound",coordinate},{"readonly",true}});
            }sample["reads"]=reads;
         }
         samples[samples.size()-1]=sample;save(output+"/trial-samples.json",QJsonDocument(samples).toJson());
      }catch(BaseException &e){if(error.isEmpty())error=QString::fromStdString(e.GetFullMessage());}
       catch(std::exception &e){if(error.isEmpty())error=e.what();}
   }
   QJsonArray samples;QString error;
private:QString output;
};
}
int main(int argc,char **argv){
   QCoreApplication app(argc,argv);
   try{
      need(argc==3,"Supply STM solver startup and frozen plan");const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");need(!output.isEmpty(),"Supply STM solver evidence directory");
      const auto plan=QJsonDocument::fromJson(read(argv[2])).object();const auto kind=plan["branch"].toString();const bool target=kind=="Target";need(target||kind=="Optimize","Unknown STM solver branch");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());auto *engine=Moderator::Instance();need(engine->Initialize(startup.toStdString(),false),"STM solver runtime failed");
      const auto ready=qEnvironmentVariable("GMAT_STM_MAP_READY"),ack=qEnvironmentVariable("GMAT_STM_MAP_ACK");if(!ready.isEmpty()){save(ready,QByteArray::number(QCoreApplication::applicationPid()));QElapsedTimer t;t.start();while(!QFile::exists(ack)&&t.elapsed()<10000)QThread::msleep(5);need(QFile::exists(ack),"STM solver native images not acknowledged");}
      const double mu=plan["mu"].toDouble(),radius=plan["radius"].toDouble(),referenceRadius=plan["reference_radius"].toDouble(),speed=std::sqrt(mu/radius);
      QString script="Earth.Mu = "+number(mu)+";\nCreate Spacecraft Sat Ref;\n";const QStringList fields{"X","Y","Z","VX","VY","VZ"};
      for(const auto &name:{QString("Sat"),QString("Ref")}){
         const double r=name=="Sat"?radius:referenceRadius,values[]={r,0,0,0,std::sqrt(mu/r),0};script+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545.125';\n";for(int i=0;i<6;++i)script+=name+"."+fields[i]+" = "+number(values[i])+";\n";
      }
      script+="Create CoordinateSystem Selected;\nSelected.Origin = Earth;\nSelected.Axes = ObjectReferenced;\nSelected.Primary = Earth;\nSelected.Secondary = Ref;\nSelected.XAxis = V;\nSelected.ZAxis = N;\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag = None;\nGravity.SRP = Off;\nGravity.RelativisticCorrection = Off;\nCreate Propagator P;\nP.FM = Gravity;\nP.Type = RungeKutta89;\nP.Accuracy = 1e-13;\nP.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\n";
      for(const auto &prefix:{QString("Trial"),QString("Final")})for(int k=0;k<5;++k){const auto name=prefix+QString::number(k);script+="Create ReportFile "+name+";\n"+name+".Filename = '"+QDir(output).filePath(name+".txt")+"';\n"+name+".WriteHeaders = false;\n"+name+".Precision = 25;\n"+name+".SolverIterations = All;\n";}
      script+="Create Variable Phase Cost Keep;\nPhase = "+number(plan["initial_phase"].toDouble())+";\nCost = 0;\nKeep = 41;\nCreate "+plan["solver_type"].toString()+" Control;\nControl.ReportFile = '"+QDir(output).filePath("solver.txt")+"';\nControl.MaximumIterations = 50;\n";
      if(!target)script+="Control.MaximumFunctionEvals = 500;\nControl.UseCentralDifferences = true;\nControl.FeasibilityTolerance = 1e-10;\nControl.OptimalityTolerance = 1e-12;\nControl.FunctionTolerance = 1e-18;\n";
      script+="BeginMissionSequence;\n"+kind+" 'SolverLoop' Control {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\nVary Control(Phase = "+number(plan["initial_phase"].toDouble())+", {Perturbation = 1e-5, Lower = 0, Upper = 0.8, MaxStep = 0.2});\nSat.EarthMJ2000Eq.X = "+number(radius)+" * cos(Phase);\nSat.EarthMJ2000Eq.Y = "+number(radius)+" * sin(Phase);\nSat.EarthMJ2000Eq.VX = -"+number(speed)+" * sin(Phase);\nSat.EarthMJ2000Eq.VY = "+number(speed)+" * cos(Phase);\nPropagate 'SolverArc' P(Sat, Ref, 'STM') {Sat.ElapsedSecs = 60, StopTolerance = 1e-6};\n";
      for(int k=0;k<5;++k)script+="Report '"+QString(k==0?"TrialFull":"TrialPart"+QString::number(k))+"' Trial"+QString::number(k)+" Sat.Selected."+types[k]+";\n";
      if(target)script+="Achieve Control(Sat.EarthMJ2000Eq.X = "+number(plan["target_x"].toDouble())+", {Tolerance = 1e-8});\nEndTarget;\n";
      else script+="Cost = (Phase - "+number(plan["solution_phase"].toDouble())+")^2;\nNonlinearConstraint Control(Phase >= 0);\nMinimize Control(Cost);\nEndOptimize;\n";
      for(int k=0;k<5;++k)script+="Report Final"+QString::number(k)+" Sat.Selected."+types[k]+";\n";script+="Keep = Keep + 1;\n";
      const auto path=QDir(output).filePath("mission.script");save(path,script.toUtf8());need(engine->InterpretScript(path.toStdString()),"STM solver mission did not build");
      const auto prototype=QString::fromStdString(engine->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::NO_COMMENTS));
      Observer observer(output);auto *previous=GmatCommand::SetExecutionObserver(&observer);struct Reset{CommandExecutionObserver *p;~Reset(){GmatCommand::SetExecutionObserver(p);}}reset{previous};
      const int result=engine->RunMission();save(output+"/observer-error.txt",observer.error.toUtf8());need(result==1,"STM solver mission failed");need(observer.error.isEmpty(),observer.error.toStdString().c_str());
      SolverBranchCommand *branch=nullptr;for(auto *command=engine->GetFirstCommand();command;command=command->GetNext())if(command->GetTypeName()==kind.toStdString())branch=dynamic_cast<SolverBranchCommand*>(command);auto *solver=branch?dynamic_cast<Solver*>(branch->GetClone(0)):nullptr;need(solver,"STM actual branch solver clone missing");
      auto *sat=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat")),*ref=dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));need(sat&&ref,"STM final spacecraft missing");const int status=solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"));
      QJsonObject proof{{"status","passed"},{"case",plan["key"]},{"branch",kind},{"solver_type",QString::fromStdString(solver->GetTypeName())},{"solver_status",status},{"converged",status==Gmat::CONVERGED},{"control",engine->GetInternalObject("Phase")->GetRealParameter("Value")},{"solver_control",solver->GetSolverVariable(0)},{"keep",engine->GetInternalObject("Keep")->GetRealParameter("Value")},{"state",state(sat)},{"reference_state",state(ref)},{"seconds",(epoch(sat)-GmatTime(21545.125)).GetTimeInSec()},{"reference_seconds",(epoch(ref)-GmatTime(21545.125)).GetTimeInSec()},{"native_stm",matrix(sat->GetRmatrixParameter("OrbitSTM"))},{"prototype_unchanged",prototype==QString::fromStdString(engine->GetConfiguredObject("Sat")->GetGeneratingString(Gmat::NO_COMMENTS))},{"samples",observer.samples},{"native_missions",1}};
      save(output+"/proof.json",QJsonDocument(proof).toJson());std::cout<<"STM solver frame witnesses completed\n";return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage();}catch(std::exception &e){std::cerr<<e.what();}return 1;
}
