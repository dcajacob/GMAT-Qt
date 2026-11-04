#pragma once
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "EditablePanel.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTreeWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace vfaccept {
inline void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
inline void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
inline QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"VF13ad artifact missing"); return file.readAll(); }
inline void write(const QString &path,const QByteArray &data) { QFile file(path); require(file.open(QIODevice::WriteOnly)&&file.write(data)==data.size(),"VF13ad artifact write failed"); }
struct Options { double initial=0,perturbation=.01,maxStep=.5,additive=0,multiplicative=1; };
inline QString number(double value) { return QString::number(value,'g',17); }
inline QString vary(const Options &o) { return "Vary 'VF control' Opt(X = "+number(o.initial)+", {Perturbation = "+number(o.perturbation)+", Lower = -100, Upper = 100, MaxStep = "+number(o.maxStep)+", AdditiveScaleFactor = "+number(o.additive)+", MultiplicativeScaleFactor = "+number(o.multiplicative)+"});"; }
using Row=std::array<double,3>;
inline std::vector<Row> rows(const QByteArray &data) {
   std::vector<Row> result;
   for (const auto &line:QString::fromUtf8(data).trimmed().split('\n')) {
      const auto columns=line.split(',');require(columns.size()==3,"VF13ad report cardinality differs");Row row;
      for(int i=0;i<3;++i){bool ok=false;row[i]=columns[i].toDouble(&ok);require(ok&&std::isfinite(row[i]),"VF13ad report nonfinite");}
      require(std::abs(row[1]-(row[0]-3)*(row[0]-3))<1e-8&&row[2]==41,"VF13ad cost/sentinel differs from independent quadratic");result.push_back(row);
   }
   return result;
}
inline QJsonArray json(const std::vector<Row> &data) { QJsonArray result;for(const auto &row:data)result.append(QJsonArray{row[0],row[1],row[2]});return result; }
inline std::vector<Row> distinctTrials(const std::vector<Row> &raw) {
   // ShowProgress performs an additional nominal evaluation for the text report.
   // Validate every raw row first, then collapse only identical adjacent tuples.
   std::vector<Row> result;
   for (const auto &row:raw) if (result.empty() || row!=result.back()) result.push_back(row);
   return result;
}
class Rig {
public:
   TestSettings settings;QTemporaryDir files;MainWindow window;QPlainTextEdit *source;QMdiArea *area;QTreeWidget *resources,*mission;
   QString trials,final,solverReport,evidence;QJsonArray results;
   explicit Rig(const QString &startup) {
      require(acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","VF13ad native platform required");require(files.isValid(),"VF13ad fixtures unavailable");QDir::setCurrent(QFileInfo(startup).absolutePath());
      trials=files.filePath("VF trials Ω.csv");final=files.filePath("VF final Ω.csv");solverReport=files.filePath("VF optimizer Ω.txt");evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      window.resize(1200,850);window.show();require(window.initialize(startup),"VF13ad runtime unavailable");acceptanceView(window);
      require(window.availableEngineTypes().contains("VF13ad"),"Registered runtime lacks VF13ad");
      source=window.findChild<QPlainTextEdit *>("scriptEditor");area=window.findChild<QMdiArea *>("workspace");resources=window.findChild<QTreeWidget *>("Resources");mission=window.findChild<QTreeWidget *>("Mission");require(source&&area&&resources&&mission,"VF13ad workspace unavailable");
   }
   ~Rig(){source->document()->setModified(false);}
   void save(const QString &name,const QByteArray &data){if(!evidence.isEmpty())write(evidence+"/"+name,data);}
   QString seed()const {
      QString script="% preserve VF13ad control fixture Ω\nCreate Variable X Cost Keep;\nX = -4;\nCost = 0;\nKeep = 41;\nCreate ReportFile Trials Final;\nTrials.Filename = '"+trials+"';\nFinal.Filename = '"+final+"';\n";
      for(const auto &name:QStringList{"Trials","Final"})script+=name+".WriteHeaders = false;\n"+name+".FixedWidth = false;\n"+name+".Delimiter = ',';\n"+name+".Precision = 25;\n"+name+".SolverIterations = All;\n";
      return script;
   }
   QString configuration(int iterations=100)const {return "Opt.MaximumIterations = "+QString::number(iterations)+";\nOpt.ShowProgress = true;\nOpt.UseCentralDifferences = false;\nOpt.Tolerance = 0.0000000001;\nOpt.FeasibilityTolerance = 0.0000000001;\nOpt.ReportFile = '"+solverReport+"';\n";}
   QString body(const Options &options)const{return "BeginMissionSequence;\nOptimize 'VF branch' Opt {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = false};\n"+vary(options)+"\nCost = (X - 3)^2;\nReport Trials X Cost Keep;\nNonlinearConstraint Opt(X <= 2);\nMinimize Opt(Cost);\nEndOptimize;\nReport Final X Cost Keep;\n% preserve VF13ad source tail\n";}
   QString script(const Options &options,int iterations=100)const{return "Create VF13ad Opt;\n"+seed()+configuration(iterations)+body(options);}
   void closePanels(){for(auto *child:area->subWindowList())if(auto *panel=dynamic_cast<EditablePanel *>(child->widget())){panel->discardChanges();child->close();}events();}
   QJsonObject run(const QString &phase,const Options &o,bool checkStep=false) {
      const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();
      const auto status=window.runMission();events();auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");if(status!=MainWindow::RunResult::Completed&&messages)save(phase+".failure.txt",messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"VF13ad witness mission failed");
      const auto trialData=read(trials),finalData=read(final);save(phase+".trials.csv",trialData);save(phase+".final.csv",finalData);save(phase+".script",before.toUtf8());save(phase+".optimizer.txt",read(solverReport));
      const auto raw=rows(trialData),t=distinctTrials(raw),f=rows(finalData);require(t.size()>=2&&f.size()==1,"VF13ad trial/final row count insufficient");require(std::abs(t[0][0]-o.initial)<1e-8,"VF13ad initial trial differs from Vary value");require(std::abs(t[1][0]-t[0][0]-o.perturbation)<1e-8,"VF13ad first finite-difference probe differs in physical units");
      require(std::abs(f[0][0]-2)<1e-6&&std::abs(f[0][1]-1)<2e-6,"VF13ad constrained quadratic optimum differs from analytic X=2 Cost=1");
      auto *command=Moderator::Instance()->GetFirstCommand();int guard=0;while(command&&!command->IsOfType("Optimize")&&++guard<100)command=command->GetNext();auto *branch=dynamic_cast<SolverBranchCommand *>(command);auto *solver=branch?dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;
      require(solver&&solver->GetTypeName()=="VF13ad"&&solver->IsSolverInternal(),"Actual VF13ad internal optimizer not executed");const int state=solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"));require(state==Gmat::CONVERGED,"VF13ad optimizer did not report convergence");const double scaled=solver->GetSolverVariable(0);require(std::abs(scaled-o.multiplicative*(f[0][0]+o.additive))<1e-7,"VF13ad native scaled variable violates documented m*(x+a)");
      double delta=0;for(std::size_t i=1;i<t.size();++i)delta=std::max(delta,std::abs(t[i][0]-t[i-1][0]));
      if(checkStep){require(delta<=o.maxStep+2*std::abs(o.perturbation)+1e-7,"VF13ad physical iteration jump exceeds configured MaxStep allowance");require(delta>=o.maxStep-2*std::abs(o.perturbation)-1e-7,"VF13ad MaxStep fixture did not activate the step limit");}
      require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"VF13ad Run changed source/history");
      QJsonObject result{{"phase",phase},{"initial",o.initial},{"perturbation",o.perturbation},{"max_step",o.maxStep},{"additive",o.additive},{"multiplicative",o.multiplicative},{"raw_trials",json(raw)},{"trials",json(t)},{"final",json(f)},{"native_scaled",scaled},{"native_status",state},{"max_adjacent_delta",delta},{"step_limit_checked",checkStep}};results.append(result);save("results-values.json",QJsonDocument(results).toJson());return result;
   }
};
}
