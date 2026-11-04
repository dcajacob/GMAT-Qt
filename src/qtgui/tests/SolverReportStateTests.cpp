#include "Moderator.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "GmatBase.hpp"
#include "Parameter.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message)
{
   if (!ok) throw std::runtime_error(message);
}
static QByteArray read(const QString &path)
{
   QFile f(path); require(f.open(QIODevice::ReadOnly),"Native solver report artifact missing"); return f.readAll();
}
static void write(const QString &path,const QByteArray &bytes)
{
   QFile f(path);require(f.open(QIODevice::WriteOnly) && f.write(bytes)==bytes.size(),"Native solver report fixture write failed");
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      require(argc==3,"Supply startup and Forward/Central reporting case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();const QString mode=argv[2];
      require(mode=="Forward" || mode=="Central","Unknown native solver reporting case");
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator=Moderator::Instance();require(moderator->Initialize(startup.toStdString(),false),"Native reporting runtime initialization failed");
      QTemporaryDir files;require(files.isValid(),"Native reporting directory unavailable");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QJsonArray observations;
      for (bool scaled:{false,true}) {
         const QString label=mode+(scaled ? "-scaled":"-unscaled");
         const auto result=files.filePath(label+".csv"),report=files.filePath(label+".txt"),scriptPath=files.filePath(label+".script");
         QString script="Create VF13ad Opt;\nCreate Variable X Y Cost Keep;\nX = 0;\nY = 0;\nCost = 0;\nKeep = 41;\n";
         script+="Opt.MaximumIterations = 100;\nOpt.ShowProgress = true;\nOpt.ReportStyle = Verbose;\nOpt.UseCentralDifferences = "+QString(mode=="Central" ? "true":"false")+";\nOpt.Tolerance = 0.0000000001;\nOpt.FeasibilityTolerance = 0.0000000001;\nOpt.ReportFile = '"+report+"';\n";
         script+="Create ReportFile Values;\nValues.Filename = '"+result+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\nOptimize Opt {ExitMode = SaveAndContinue, ShowProgressWindow = false};\n";
         script+="Vary Opt(X = 0, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 0.5, AdditiveScaleFactor = "+QString(scaled ? "5":"0")+", MultiplicativeScaleFactor = "+QString(scaled ? "0.2":"1")+"});\n";
         script+="Vary Opt(Y = 0, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 0.5, AdditiveScaleFactor = "+QString(scaled ? "-4":"0")+", MultiplicativeScaleFactor = "+QString(scaled ? "3":"1")+"});\n";
         script+="Cost = (X - 3)^2 + (Y + 2)^2;\nNonlinearConstraint Opt(X <= 2);\nNonlinearConstraint Opt(Y >= -1);\nMinimize Opt(Cost);\nEndOptimize;\nReport Values X Y Cost Keep;\n";
         write(scriptPath,script.toUtf8());require(moderator->InterpretScript(scriptPath.toStdString()),"Native reporting script did not interpret");
         require(moderator->RunMission()==1,"Native reporting mission failed");
         const auto data=read(result),text=read(report);
         if (!evidence.isEmpty()) {write(evidence+"/"+label+".script",script.toUtf8());write(evidence+"/"+label+".csv",data);write(evidence+"/"+label+".txt",text);}
         const auto rows=QString::fromUtf8(data).trimmed().split('\n');require(rows.size()==1,"Native final reporting row count differs");
         const auto values=rows[0].split(',');require(values.size()==4,"Native final reporting column count differs");
         const double x=values[0].toDouble(),y=values[1].toDouble(),cost=values[2].toDouble();
         require(std::isfinite(x) && std::isfinite(y) && std::isfinite(cost) && std::abs(x-2)<1e-6 && std::abs(y+1)<1e-6 && std::abs(cost-2)<2e-6 && values[3].toDouble()==41,"Native result differs from independent constrained minimum (2,-1,2)");
         auto *command=moderator->GetFirstCommand();while(command && !command->IsOfType("Optimize")) command=command->GetNext();
         auto *branch=dynamic_cast<SolverBranchCommand *>(command);auto *solver=branch ? dynamic_cast<Solver *>(branch->GetClone(0)):nullptr;
         require(solver && solver->GetTypeName()=="VF13ad" && solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED,"Native reporting solver did not converge");
         require(std::abs(solver->GetSolverVariable(0)-(scaled ? .2*(x+5):x))<1e-7 && std::abs(solver->GetSolverVariable(1)-(scaled ? 3*(y-4):y))<1e-7,"Reporting changed native scaled decision variables");
         const QRegularExpression section("Final Variable values:\\s*X\\s*=\\s*([-+0-9.eE]+)\\s*Y\\s*=\\s*([-+0-9.eE]+)");
         auto matches=section.globalMatch(QString::fromUtf8(text));int sections=0;
         while(matches.hasNext()) {
            const auto match=matches.next();++sections;
            require(std::abs(match.captured(1).toDouble()-x)<1e-6 && std::abs(match.captured(2).toDouble()-y)<1e-6,"Final solver report retained a finite-difference probe or scaled value");
         }
         require(sections>0,"Native final variable report absent");
         auto *configured=dynamic_cast<Parameter *>(moderator->GetConfiguredObject("X"));require(configured && configured->EvaluateReal()==0,"Reporting changed the configured variable graph");
         require(read(scriptPath)==script.toUtf8(),"Reporting changed the source file");
         observations.append(QJsonObject{{"case",label},{"x",x},{"y",y},{"cost",cost},{"final_sections",sections},{"tolerance",1e-6}});
      }
      if (!evidence.isEmpty()) write(evidence+"/results.json",QJsonDocument(observations).toJson());
      std::cout<<"PASS native VF13ad "<<mode.toStdString()<<" reporting: two variables, unscaled and nonidentity additive/multiplicative scaling, independent constrained optimum and every final report block; source/configured graph retained.\n";
      moderator->Finalize();return 0;
   } catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}
     catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<'\n';}
   return 1;
}
