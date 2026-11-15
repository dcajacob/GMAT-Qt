// Collect the missing estimator/CK conjunction. Numerical acceptance is in
// AuditEstimatorCkEvidence.py, independently of the native writer/parser.
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Debugger.hpp"
#include "MissionModel.hpp"
#include "Moderator.hpp"
#include "GmatCommand.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "Estimator.hpp"
#include "CoordinateConverter.hpp"
#include "CoordinateSystem.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include "SpiceUsr.h"
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
QByteArray read(const QString &path) {QFile f(path);require(f.open(QIODevice::ReadOnly),"CK observation artifact missing");return f.readAll();}
void write(const QString &path,const QByteArray &data) {QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Cannot preserve CK observations");}
void spiceCheck() {if(failed_c()){char message[2048];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);}}
class CkStream final:public Subscriber {
public:
   QString error;
   std::function<void(const QJsonObject &)> sample;
   CkStream():Subscriber("CkStream","CkStream"){}
   ~CkStream() override {Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone() const override {return new CkStream(*this);}
   bool ReceiveData(const Real *data,Integer count=0) override {
      if(!error.isEmpty())return true;
      try {
         if(!data||count<7||theDataLabels.empty()||!sample||!currentProvider)return true;
         const auto provider=currentProvider->GetTypeName();
         if(provider!="RunEstimator"&&provider!="RunSmoother")return true;
         const auto &labels=theDataLabels[0];
         const char *names[]{"EstSat.X","EstSat.Y","EstSat.Z","EstSat.Vx","EstSat.Vy","EstSat.Vz",
                             "EstSat.Q1","EstSat.Q2","EstSat.Q3","EstSat.Q4"};
         double values[10];
         for(int i=0;i<10;++i) {
            const auto it=std::find(labels.begin(),labels.end(),names[i]);
            require(it!=labels.end()&&it-labels.begin()<count,"CK publication lacks target Cartesian/quaternion column");
            values[i]=data[it-labels.begin()];require(std::isfinite(values[i]),"Nonfinite CK publication");
         }
         auto *frame=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject("EarthMJ2000Eq"));
         require(frame&&theDataCoordSystem,"CK publication frame context missing");
         Rvector6 state(values),converted;CoordinateConverter converter;
         converter.Convert(A1Mjd(data[0]),state,theDataCoordSystem,converted,frame);
         QJsonArray cartesian,quaternion;
         for(int i=0;i<6;++i)cartesian.append(converted[i]);
         for(int i=6;i<10;++i)quaternion.append(values[i]);
         auto *solver=dynamic_cast<Estimator *>(currentProvider->GetRefObject(Gmat::SOLVER,currentProvider->GetRefObjectName(Gmat::SOLVER)));
         require(solver,"CK native estimator clone missing");
         sample(QJsonObject{{"epoch",data[0]},{"state",cartesian},{"quaternion",quaternion},
            {"provider",QString::fromStdString(provider)},{"run_state",int(Publisher::Instance()->GetRunState())},
            {"final_pass",solver->IsFinalPass()},{"estimator_state",solver->GetState()}});
      }catch(BaseException &e){error=QString::fromStdString(e.GetFullMessage());}
       catch(std::exception &e){error=QString::fromUtf8(e.what());}
      return true;
   }
protected:
   bool Distribute(Integer) override {return true;}
   bool Distribute(const Real *,Integer) override {return true;}
};
}

int main(int argc,char **argv) {
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("EstimatorCkObservations");
   try {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, estimator CK fixture and family");TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(),"CK observation directory required");QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();
      require(fixture["family"]==QString(argv[3]),"Wrong CK estimator family");
      QString script=fixture["script"].toString();script.replace("@OUTPUT@",output);
      require(!script.contains('@'),"Unexpanded estimator CK fixture");write(output+"/mission.script",script.toUtf8());
      MainWindow window;require(window.initialize(startup),"Estimator CK initialization failed");
      window.show();acceptanceView(window);QApplication::processEvents();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");
      require(editor&&messages,"Estimator CK workspace missing");editor->setPlainText(script);
      const bool built=window.buildScript();write(output+"/build-messages.txt",messages->toPlainText().toUtf8());
      require(built,"Estimator CK native build failed");
      const auto saved=output+"/Estimator CK Δ.script";
      require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&editor->toPlainText()==script,"Estimator CK Unicode save/reopen failed");
      Debugger *debug=nullptr;for(auto *child:window.children())if(auto *candidate=dynamic_cast<Debugger *>(child))debug=candidate;
      require(debug,"Estimator CK debugger missing");
      QMap<int,QString> boundaries;const auto snapshot=window.missionSnapshot();
      for(int i=0;i<snapshot.nodes.size();++i)for(const auto &value:fixture["boundaries"].toArray()) {
         const auto label=value.toString();
         if(missionCommandLabel(snapshot.nodes[i].statement).name==label) {
            require(window.setBreakpoint(i,true),"CK phase breakpoint failed");boundaries[i]=label;
         }
      }
      require(boundaries.size()==fixture["boundaries"].toArray().size(),"CK native phase census differs");
      CkStream observer;bool armed=false;QString phase="unarmed";QJsonArray raw,phases;std::exception_ptr failure;
      observer.sample=[&](QJsonObject sample){sample["phase"]=phase;raw.append(sample);};
      const auto previous=debug->paused;
      debug->paused=[&](const QString &type) {
         if(previous)previous(type);
         try {
            auto *dialog=window.findChild<QDialog *>("debuggerDialog");require(dialog&&type=="Report","CK phase is not its native Report");
            const auto label=boundaries.value(dialog->property("commandIndex").toInt());require(!label.isEmpty(),"Unknown CK phase");
            phase=fixture["phase_names"].toObject()[label].toString();require(!phase.isEmpty(),"CK phase has no identity");phases.append(label);
            if(!armed) {
               observer.SetInternalCoordSystem(Publisher::Instance()->GetInternalCoordSystem());
               if(auto *frame=Publisher::Instance()->GetDataCoordSystem())observer.SetDataCoordSystem(frame);
               require(Publisher::Instance()->Subscribe(&observer),"CK raw observer attach failed");armed=true;
            }
            acceptanceView(*dialog);
         }catch(...){failure=std::current_exception();window.stopMission();}
         QTimer::singleShot(0,&window,[&]{debug->resume();});
      };
      const int undo=editor->document()->availableUndoSteps();messages->clear();
      const auto outcome=window.runMission();debug->paused=previous;Publisher::Instance()->Unsubscribe(&observer);QApplication::processEvents();
      write(output+"/raw-publications.json",QJsonDocument(raw).toJson());write(output+"/messages.txt",messages->toPlainText().toUtf8());
      write(output+"/observer-error.txt",observer.error.toUtf8());
      if(failure)std::rethrow_exception(failure);
      require(observer.error.isEmpty(),"Estimator CK observer failed");
      require(outcome==MainWindow::RunResult::Completed,"Estimator CK mission failed");
      require(editor->toPlainText()==script&&editor->document()->availableUndoSteps()==undo,"Estimator CK changed source/history");
      require(phases==fixture["boundaries"].toArray(),"Estimator CK phase order differs");
      erract_c("SET",0,const_cast<char *>("RETURN"));
      SpiceDouble et0;str2et_c((fixture["utc_epoch"].toString()+" UTC").toUtf8().constData(),&et0);spiceCheck();
      const double tt0=unitim_c(et0,"ET","TDT");spiceCheck();
      SpiceInt handle;dafopr_c((output+"/solution.bc").toUtf8().constData(),&handle);spiceCheck();dafbfs_c(handle);
      SpiceBoolean found;daffna_c(&found);spiceCheck();QJsonArray segments;
      while(found) {
         SpiceDouble summary[125],endpoints[2];SpiceInt integers[6];dafgs_c(summary);dafus_c(summary,2,6,endpoints,integers);spiceCheck();
         require(integers[4]>0&&integers[5]>=integers[4]&&integers[5]-integers[4]<100000,"Invalid observed CK addresses");
         std::vector<double> words(integers[5]-integers[4]+1);dafgda_c(handle,integers[4],integers[5],words.data());spiceCheck();
         require(words.size()>=6,"Incomplete observed CK segment");const int n=static_cast<int>(words.back());
         require(n>0&&n<10000&&words.size()>=static_cast<size_t>(5*n+5),"Invalid observed CK count");
         QJsonArray packets;
         for(int i=0;i<n;++i) {
            SpiceDouble et;sct2e_c(-100057,words[4*n+i],&et);spiceCheck();QJsonArray quaternion;
            for(int j=0;j<4;++j)quaternion.append(words[4*i+j]);
            const double tt=unitim_c(et,"ET","TDT");spiceCheck();
            packets.append(QJsonObject{{"tick",words[4*n+i]},{"et",et},{"elapsed",tt-tt0},{"quaternion",quaternion}});
         }
         segments.append(QJsonObject{{"instrument",integers[0]},{"frame",integers[1]},{"type",integers[2]},{"packets",packets},
                                    {"first",endpoints[0]},{"last",endpoints[1]}});
         daffna_c(&found);spiceCheck();
      }
      dafcls_c(handle);spiceCheck();
      const QJsonObject observations{{"status","observed-native-completion;independent-audit-required"},{"family",fixture["family"]},
                                    {"segments",segments},{"raw_count",raw.size()},{"phase_boundaries",phases},{"new_requirement_credit",0}};
      write(output+"/observations.json",QJsonDocument(observations).toJson());
      std::cout<<"NASA_OBSERVATION "<<QJsonDocument(observations).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}
    catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<'\n';}
   return 1;
}
