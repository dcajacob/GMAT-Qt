// SPDX-License-Identifier: Apache-2.0
#include "PovModel.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Publisher.hpp"
#include "Parameter.hpp"
#include "GmatCommand.hpp"
#include "Minimize.hpp"
#include "CCSDSOEMWriter.hpp"
#include "FileManager.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTimer>
#include <osg/Geode>
#include <osg/NodeVisitor>
#include <osg/Transform>
#include <osg/TriangleFunctor>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>

namespace {
void require(bool value,const std::string &message)
{ if (!value) throw std::runtime_error(message); }
void write(const QString &path,const QByteArray &bytes)
{ QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Cannot write verification fixture"); }
QByteArray read(const QString &path)
{ QFile f(path); require(f.open(QIODevice::ReadOnly),"Cannot read verification output"); return f.readAll(); }
using Face=std::array<osg::Vec3d,3>;
struct Faces {
   std::vector<Face> *faces=nullptr;
   osg::Matrixd matrix;
   void operator()(const osg::Vec3 &a,const osg::Vec3 &b,const osg::Vec3 &c)
   { faces->push_back({osg::Vec3d(a)*matrix,osg::Vec3d(b)*matrix,osg::Vec3d(c)*matrix}); }
};
struct Mesh final : osg::NodeVisitor {
   std::vector<Face> faces;
   Mesh():osg::NodeVisitor(TRAVERSE_ALL_CHILDREN) {}
   void apply(osg::Geode &geode) override {
      for (unsigned i=0;i<geode.getNumDrawables();++i) {
         osg::TriangleFunctor<Faces> collector;
         collector.faces=&faces; collector.matrix=osg::computeLocalToWorld(getNodePath());
         geode.getDrawable(i)->accept(collector);
      }
      traverse(geode);
   }
   double volume() const {
      double result=0;
      for (const auto &f:faces) {
         require(((f[1]-f[0])^(f[2]-f[0])).length2()>1e-18,"Degenerate output triangle");
         for (const auto &p:f) require(std::isfinite(p.x())&&std::isfinite(p.y())&&std::isfinite(p.z()),"Nonfinite output vertex");
         result+=f[0]*(f[1]^f[2])/6.;
      }
      return std::abs(result);
   }
};
QJsonArray geometry(const QString &directory)
{
   QJsonArray results;
   auto load=[&](const QString &name,const QByteArray &source) {
      const auto path=directory+'/'+name+".pov"; write(path,source);
      QString error; auto node=readPovModel(path,&error);
      require(node.valid(),name.toStdString()+": "+error.toStdString()); return node;
   };
   auto check=[&](const QString &name,const QByteArray &source,double expected,double relative=1e-6) {
      auto node=load(name,source); Mesh mesh; node->accept(mesh);
      const double observed=mesh.volume();
      require(std::abs(observed-expected)<=relative*std::max(1.,expected),name.toStdString()+" volume "+std::to_string(observed)+" expected "+std::to_string(expected));
      results.append(QJsonObject{{"case",name},{"triangles",int(mesh.faces.size())},{"volume",observed},{"expected_volume",expected},{"relative_tolerance",relative}});
   };
   check("BoxDifference","difference { box { <-1,-1,-1>, <1,1,1> } box { <0,0,0>, <2,2,2> } }",7);
   check("BoxIntersection","intersection { box { <-1,-1,-1>, <1,1,1> } box { <0,0,0>, <2,2,2> } }",1);
   check("BoxMerge","merge { box { <-1,-1,-1>, <1,1,1> } box { <0,0,0>, <2,2,2> } }",15);
   check("TransformedDifference","difference { box { <-1,-1,-1>, <1,1,1> } box { <0,0,0>, <2,2,2> } scale <2,3,4> rotate <17,31,9> translate <7,8,9> }",168,2e-5);
   check("RingTorus","torus { 3, 1 }",2*std::acos(-1.)*std::acos(-1.)*3,.011);
   check("HornTorus","torus { 1, 1 }",2*std::acos(-1.)*std::acos(-1.),.011);
   // Integrate circular cross-sections independently of the tessellator.
   const double major=.5,minor=1.; double outer=0,inner=0;
   for (int i=0;i<20000;++i) {
      const double y=-minor+(i+.5)*2*minor/20000.;
      const double radius=std::sqrt(minor*minor-y*y),hole=std::max(0.,major-radius);
      outer+=std::acos(-1.)*((major+radius)*(major+radius)-hole*hole)*2*minor/20000.;
      inner+=std::acos(-1.)*std::pow(std::max(0.,radius-major),2)*2*minor/20000.;
   }
   for (const auto &mode:QStringList{"union","merge","difference","intersection"}) {
      const double expected=mode=="intersection" ? inner : mode=="difference" ? outer-inner : outer;
      const auto source=QString("intersection { torus { 0.5, 1 %1 } box { <-5,-5,-5>, <5,5,5> } }").arg(mode).toUtf8();
      check("Spindle-"+mode,source,expected,.025);
   }
   for (const auto &source:QList<QByteArray>{"intersection { box { <0,0,0>, <1,1,1> } box { <2,2,2>, <3,3,3> } }","torus { 2, 1 intersection }","torus { 1, 0 }"}) {
      auto node=load("Empty"+QString::number(results.size()),source); Mesh mesh;node->accept(mesh);bool empty=false;
      require(mesh.faces.empty()&&node->getUserValue("gmat.pov.emptyGeometry",empty)&&empty,"Empty solid lost its explicit empty marker");
      results.append(QJsonObject{{"case","IntentionalEmpty"},{"triangles",0}});
   }
   return results;
}
QJsonArray numeric(const QString &startup)
{
   auto *manager=FileManager::Instance();manager->ReadStartupFile(startup.toStdString());
   LeapSecsFileReader leap(manager->GetFullPathname("LEAP_SECS_FILE"));
   require(leap.Initialize(),"OEM numeric fixture leap-second table failed");
   TimeSystemConverter::Instance()->SetLeapSecsFileReader(&leap);
   QJsonArray results;
   const double bad[]={std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()};
   for (int field=-1;field<6;++field) for (double value:bad) {
      CCSDSOEMWriter writer("1.0"); Rvector good(6,7000.,1.,2.,3.,4.,5.);writer.AddDataForWriting(21545.,good);
      Rvector state(good); double epoch=21546.; if (field<0) epoch=value;else state[field]=value;
      bool rejected=false;try {writer.AddDataForWriting(epoch,state);}catch(BaseException &) {rejected=true;}
      require(rejected,"Nonfinite OEM decimal field accepted");
      require(writer.GetNumberOfSerializedDataPoints()==1,"Invalid OEM record changed valid buffered records");
      writer.AddDataForWriting(21547.,good);require(writer.GetNumberOfSerializedDataPoints()==2,"Writer failed to recover after rejected record");
      results.append(QJsonObject{{"field",field},{"rejected",true},{"valid_records_after_recovery",2}});
   }
   CCSDSOEMWriter writer("1.0"); Rvector shortState(5);bool rejected=false;
   try {writer.AddDataForWriting(21545.,shortState);}catch(BaseException &) {rejected=true;}
   require(rejected&&writer.GetNumberOfSerializedDataPoints()==0,"Short OEM vector was buffered");
   results.append(QJsonObject{{"case","ShortVector"},{"rejected",true}});return results;
}
double runtimeValue(const char *name)
{
   auto *p=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject(name));
   require(p,std::string("Missing runtime value ")+name);return p->EvaluateReal();
}
struct Window {
   MainWindow window;
   QPlainTextEdit *source=nullptr,*messages=nullptr;
   explicit Window(const QString &startup) {
      require(window.initialize(startup),"Verification runtime initialization failed");window.show();
      source=window.findChild<QPlainTextEdit *>("scriptEditor");messages=window.findChild<QPlainTextEdit *>("messageWindow");
      require(source&&messages,"Verification workspace unavailable");
   }
   ~Window() { if(source)source->document()->setModified(false); }
   bool build(const QString &text) {
      source->setPlainText(text);const bool result=window.buildScript();
      if(!result)std::cerr<<messages->toPlainText().toStdString();return result;
   }
};
QString solverSetup(const QString &directory)
{
   return "Create Yukon Opt;\nOpt.ShowProgress = false;\nOpt.MaximumIterations = 100;\nOpt.FeasibilityTolerance = 1e-9;\nOpt.OptimalityTolerance = 1e-9;\nOpt.FunctionTolerance = 1e-9;\nOpt.ReportFile = '"+directory+"/optimizer.txt';\n";
}
QJsonArray costForm(const QString &startup,const QString &directory)
{
   Window rig(startup);QJsonArray result;
   for(const auto &keyword:QStringList{"NonlinearConstraint","Minimize"}) {
      const auto objective=keyword+" 'Cost spelling' Opt(Cost);";
      const auto script="Create Variable X Cost Keep;\nX = 0;\nCost = 0;\nKeep = 41;\n"+solverSetup(directory)+
         "BeginMissionSequence;\nOptimize Opt {ShowProgressWindow = false, ExitMode = SaveAndContinue};\nVary Opt(X = 0, {Perturbation = 0.000001, Lower = -10, Upper = 10, MaxStep = 10});\nCost = (X - 3)^2;\nNonlinearConstraint Opt(X <= 2);\n"+objective+"\nEndOptimize;\nKeep = Keep + 1;\n";
      require(rig.build(script),"Cost spelling failed to build");
      std::vector<GmatCommand *> pending{Moderator::Instance()->GetFirstCommand()};std::set<GmatCommand *> seen;int minima=0,constraints=0;
      while(!pending.empty()) {
         auto *command=pending.back();pending.pop_back();if(!command||!seen.insert(command).second)continue;
         pending.push_back(command->GetNext());pending.push_back(command->GetChildCommand(0));
         if(command->GetTypeName()=="NonlinearConstraint")++constraints;
         if(auto *minimum=dynamic_cast<Minimize *>(command)) {
            ++minima;const auto expected=keyword.toStdString()+" 'Cost spelling' Opt(Cost);";
            require(minimum->GetGeneratingString(Gmat::SCRIPTING,"","")==expected,"Native objective lost spelling or label");
            std::unique_ptr<GmatBase> copy(minimum->Clone());
            require(copy->GetGeneratingString(Gmat::SCRIPTING)==expected,"Objective clone lost spelling or label");
         }
      }
      require(minima==1&&constraints==1,"Cost alias changed native command roles");
      const int undo=rig.source->document()->availableUndoSteps();
      const auto status=rig.window.runMission();if(status!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();
      require(status==MainWindow::RunResult::Completed,"Cost spelling mission failed");
      const double x=runtimeValue("X"),cost=runtimeValue("Cost");
      require(std::abs(x-2)<2e-5&&std::abs(cost-1)<2e-5&&runtimeValue("Keep")==42,"Cost spelling missed analytic constrained optimum X=2, cost=1");
      require(rig.source->toPlainText()==script&&rig.source->document()->availableUndoSteps()==undo,"Run changed source/history");
      write(directory+'/'+keyword+".script",script.toUtf8());
      result.append(QJsonObject{{"keyword",keyword},{"x",x},{"cost",cost},{"native_objectives",minima},{"native_constraints",constraints}});
   }
   return result;
}
QJsonArray functionControl(const QString &startup,const QString &directory,bool stop)
{
   const auto function=directory+"/Slow.gmf";
   write(function,"function [out] = Slow(limit)\nCreate Variable k out;\nGlobal Progress;\nBeginMissionSequence;\nout = 0;\nFor k = 1:limit\nProgress = Progress + 1;\nout = out + 1;\nEndFor;\n");
   const auto script="Create Variable X Cost Result Progress Limit Keep;\nX = 0;\nCost = 0;\nProgress = 0;\nLimit = 40000;\nKeep = 41;\nCreate GmatFunction Slow;\nSlow.FunctionPath = '"+function+"';\n"+solverSetup(directory)+
      "BeginMissionSequence;\nGlobal Progress;\nOptimize Opt {ShowProgressWindow = false, ExitMode = SaveAndContinue};\nVary Opt(X = 0, {Perturbation = 0.000001, Lower = -10, Upper = 10, MaxStep = 10});\nProgress = 0;\n[Result] = Slow(Limit);\nCost = (X - 2)^2;\nMinimize Opt(Cost);\nEndOptimize;\nKeep = Keep + 1;\n";
   Window rig(startup);require(rig.build(script),"Solver-context function fixture failed to build");
   auto *pause=rig.window.findChild<QAction *>("pauseMission"),*halt=rig.window.findChild<QAction *>("stopMission"),*resume=rig.window.findChild<QAction *>("runMission");
   require(pause&&halt&&resume,"Actual run actions unavailable");
   QTimer timer;timer.setInterval(5);QJsonArray observations;std::exception_ptr failure;int phase=0;double held=0;
   QObject::connect(&timer,&QTimer::timeout,&rig.window,[&] {
      if(phase||!rig.window.isRunning())return;
      const double progress=runtimeValue("Progress");if(progress<=0||progress>=40000)return;
      try {
         const auto state=Publisher::Instance()->GetRunState();
         require(state==Gmat::SOLVING||state==Gmat::SOLVEDPASS,"Run control did not occur during solver publication");
         phase=1;held=progress;observations.append(QJsonObject{{"action",stop?"stop":"pause"},{"progress",progress},{"publisher_state",int(state)}});
         if(stop)halt->trigger();else {
            pause->trigger();require(Moderator::Instance()->GetRunState()==Gmat::PAUSED,"Pause did not change native run state");
            QTimer::singleShot(120,&rig.window,[&] {
               try {require(runtimeValue("Progress")==held&&Moderator::Instance()->GetRunState()==Gmat::PAUSED,"Solver function advanced while paused");resume->trigger();phase=2;observations.append(QJsonObject{{"action","resume"},{"held_progress",held}});}
               catch(...){failure=std::current_exception();rig.window.stopMission();}
            });
         }
      }catch(...){failure=std::current_exception();rig.window.stopMission();}
   });
   timer.start();const auto status=rig.window.runMission();timer.stop();
   write(directory+"/mission.script",script.toUtf8());write(directory+"/messages.txt",rig.messages->toPlainText().toUtf8());
   if(failure)std::rethrow_exception(failure);
   require(phase>0,"No run action was serviced inside the solver function");
   require(status==(stop?MainWindow::RunResult::Stopped:MainWindow::RunResult::Completed),"Wrong solver-function completion state");
   require(runtimeValue("Keep")== (stop?41:42),"Stop/resume violated following-command sentinel");
   if(!stop)require(phase==2&&runtimeValue("Progress")==40000&&std::abs(runtimeValue("X")-2)<2e-5,"Resumed solver function missed its final result");
   observations.append(QJsonObject{{"action","finished"},{"stopped",stop},{"keep",runtimeValue("Keep")}});return observations;
}
QJsonArray functionSyntax(const QString &startup,const QString &directory)
{
   const auto path=directory+"/Twice.gmf";
   write(path,"function [out] = Twice(input)\nCreate Variable out;\nBeginMissionSequence;\nout = input * 2;\n");
   Window rig(startup);QJsonArray result;
   for(bool strict:{false,true})for(bool brackets:{false,true}) {
      const QString call=brackets?"[Y] = Twice(X);":"Y = Twice(X);";
      const auto script="Create Variable X Y;\nX = 3;\nY = 0;\nCreate GmatFunction Twice;\nTwice.FunctionPath = '"+path+"';\nTwice.RequireOutputBrackets = "+(strict?"true":"false")+";\nBeginMissionSequence;\n"+call+'\n';
      const bool built=rig.build(script),accepted=!strict||brackets;
      require(built==accepted,"Native function bracket-policy acceptance differs");
      if(accepted)require(rig.window.runMission()==MainWindow::RunResult::Completed&&runtimeValue("Y")==6,"Native function output syntax lost its scalar result");
      result.append(QJsonObject{{"strict",strict},{"brackets",brackets},{"accepted",built}});
   }
   return result;
}
double luminance(const QColor &c)
{
   auto channel=[](int value){const double v=value/255.;return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);};
   return .2126*channel(c.red())+.7152*channel(c.green())+.0722*channel(c.blue());
}
QJsonArray toolbar(const QString &startup)
{
   Window rig(startup);QJsonArray result;
   for(const auto &background:QList<QColor>{QColor(36,36,36),QColor(245,245,245)}) {
      auto palette=rig.window.palette();palette.setColor(QPalette::Window,background);
      for(auto group:{QPalette::Active,QPalette::Disabled})palette.setColor(group,QPalette::ButtonText,background);
      rig.window.setPalette(palette);
      for(const auto *name:{"runMission","pauseMission","stopMission"})for(auto mode:{QIcon::Normal,QIcon::Disabled}) {
         auto *action=rig.window.findChild<QAction *>(name);require(action,"Toolbar transport action missing");
         const auto image=action->icon().pixmap(QSize(32,32),mode).toImage();int ink=0;double minimum=100;
         for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
            const auto color=image.pixelColor(x,y);if(color.alpha()!=255)continue;
            ++ink;const double a=luminance(color),b=luminance(background);
            minimum=std::min(minimum,(std::max(a,b)+.05)/(std::min(a,b)+.05));
         }
         require(ink>=40&&minimum>=(mode==QIcon::Disabled?3.:4.5),"Rendered toolbar silhouette has insufficient contrast");
         result.append(QJsonObject{{"action",name},{"background",background.name()},{"disabled",mode==QIcon::Disabled},{"opaque_ink_pixels",ink},{"minimum_contrast",minimum}});
      }
   }
   return result;
}
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("ImplementationVerification");
   try {
      require(argc==3,"Supply startup and verification case");TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),mode=QString::fromLocal8Bit(argv[2]);QDir::setCurrent(QFileInfo(startup).absolutePath());
      QTemporaryDir scratch;require(scratch.isValid(),"Cannot create verification directory");
      const auto retained=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      const auto directory=retained.isEmpty()?scratch.path():retained;
      QJsonArray cases;
      if(mode=="PovGeometry")cases=geometry(directory);
      else if(mode=="OemNumeric")cases=numeric(startup);
      else if(mode=="CostForm")cases=costForm(startup,directory);
      else if(mode=="FunctionSolverStop"||mode=="FunctionSolverPauseResume")cases=functionControl(startup,directory,mode=="FunctionSolverStop");
      else if(mode=="FunctionSyntax")cases=functionSyntax(startup,directory);
      else if(mode=="ToolbarContrast")cases=toolbar(startup);
      else throw std::runtime_error("Unknown verification case");
      const QJsonObject proof{{"case",mode},{"status","passed"},{"observations",cases},{"new_requirement_credit",0},{"whole_row_qualified",false}};
      write(directory+"/verification.json",QJsonDocument(proof).toJson());
      std::cout<<mode.toStdString()<<" passed: "<<cases.size()<<" observations\n";return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<'\n';}
   catch(std::exception &e){std::cerr<<e.what()<<'\n';}
   return 1;
}
