// Supplementary CK final-pass witness. Original OEM tests remain unchanged.
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "SolverBranchCommand.hpp"
#include "Solver.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include "TestSettings.hpp"
#include "SpiceUsr.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

namespace {
void require(bool value, const char *message) {
   if (!value) throw std::runtime_error(message);
}
QByteArray read(const QString &path) {
   QFile file(path); require(file.open(QIODevice::ReadOnly), "CK witness artifact missing");
   return file.readAll();
}
void write(const QString &path, const QByteArray &bytes) {
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(), "Cannot preserve CK witness");
}
void spiceCheck() {
   if (!failed_c()) return;
   char message[2048]; getmsg_c("LONG", sizeof(message), message); reset_c();
   throw std::runtime_error(message);
}
QJsonArray reportRows(const QString &path) {
   QJsonArray rows;
   for (const auto &line:QString::fromUtf8(read(path)).trimmed().split('\n')) {
      QJsonArray row;
      for (const auto &cell:line.trimmed().split(',')) {
         bool ok=false; const double value=cell.toDouble(&ok);
         require(ok && std::isfinite(value), "Nonfinite CK trial report"); row.append(value);
      }
      rows.append(row);
   }
   return rows;
}
}

int main(int argc, char **argv) {
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("SolverCkOutput");
   try {
      require(argc==4 && acceptanceWidgetPlatform(), "Supply startup, frozen CK fixtures and case");
      TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(), output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "CK evidence directory required");
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      const auto fixture=QJsonDocument::fromJson(read(argv[2])).object();
      QJsonObject spec;
      for (const auto &value:fixture["cases"].toArray())
         if (value.toObject()["key"]==QString(argv[3])) spec=value.toObject();
      require(!spec.isEmpty(), "CK case missing");
      QString script=spec["script"].toString(); script.replace("@OUTPUT@",output);
      require(!script.contains('@'), "Unexpanded CK fixture"); write(output+"/mission.script",script.toUtf8());
      MainWindow window; require(window.initialize(startup), "CK runtime initialization failed");
      window.show(); acceptanceView(window); QApplication::processEvents();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"), *messages=window.findChild<QPlainTextEdit *>("messageWindow");
      require(editor && messages, "CK workspace missing"); editor->setPlainText(script);
      const bool built=window.buildScript(); write(output+"/build-messages.txt",messages->toPlainText().toUtf8());
      require(built, "Native Quaternion CK script build failed");
      auto *resource=Moderator::Instance()->GetConfiguredObject("Export");
      require(resource && resource->GetStringParameter("StateType")=="Quaternion" && resource->GetStringParameter("FileFormat")=="CK", "Script did not configure Quaternion CK");
      editor->document()->setModified(false);
      const QString configured=QString::fromStdString(resource->GetGeneratingString(Gmat::SCRIPTING));
      const auto saved=output+"/CK solution Δ.script";
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==script, "CK authored Unicode save/reopen failed");
      const int undo=editor->document()->availableUndoSteps();
      messages->clear(); const auto outcome=window.runMission(); QApplication::processEvents();
      write(output+"/messages.txt",messages->toPlainText().toUtf8());
      require(outcome==MainWindow::RunResult::Completed, "CK solver mission failed");
      require(editor->toPlainText()==script && !editor->document()->isModified() && editor->document()->availableUndoSteps()==undo, "CK run changed authored source/history");
      resource=Moderator::Instance()->GetConfiguredObject("Export");
      require(QString::fromStdString(resource->GetGeneratingString(Gmat::SCRIPTING))==configured, "CK run changed configured output");
      GmatCommand *command=Moderator::Instance()->GetFirstCommand(); std::set<GmatCommand *> seen;
      while (command && seen.insert(command).second && command->GetTypeName()!=spec["key"].toString().toStdString()) command=command->GetNext();
      auto *branch=dynamic_cast<SolverBranchCommand *>(command);
      auto *solver=branch ? dynamic_cast<Solver *>(branch->GetClone(0)) : nullptr;
      const double controlBound=fixture["control_tolerance"].toDouble();
      require(solver && solver->GetTypeName()==spec["solver_type"].toString().toStdString() &&
         solver->GetIntegerParameter(solver->GetParameterID("IntegerSolverStatus"))==Gmat::CONVERGED &&
         std::abs(solver->GetSolverVariable(0)-2)<controlBound, "CK solver clone did not converge to the frozen control");
      const auto trials=reportRows(output+"/trials.csv"); int nonfinal=0;
      double trialError=0;
      for (const auto &value:trials) {
         const auto row=value.toArray(); require(row.size()==17, "CK trial report shape differs");
         const double u=row[0].toDouble(), elapsed=row[1].toDouble(), angle=(10+20*u)*std::acos(-1.)/180.;
         const double matrix[9]{std::cos(angle),std::sin(angle),0,-std::sin(angle),std::cos(angle),0,0,0,1};
         require(std::abs(elapsed-120)<1e-4, "CK trial elapsed duration differs");
         for (int j=0;j<9;++j) require(std::abs(row[j+2].toDouble()-matrix[j])<fixture["matrix_tolerance"].toDouble(), "CK trial attitude differs from independent Euler rotation");
         const double radius=7000+100*u, rate=std::sqrt(398600.4415/(radius*radius*radius)), speed=std::sqrt(398600.4415/radius);
         const double state[6]{radius*std::cos(rate*elapsed),radius*std::sin(rate*elapsed),0,-speed*std::sin(rate*elapsed),speed*std::cos(rate*elapsed),0};
         for (int j=0;j<6;++j) {
            const double error=std::abs(row[j+11].toDouble()-state[j]); trialError=std::max(trialError,error);
            require(error<fixture[j<3 ? "position_tolerance" : "velocity_tolerance"].toDouble(), "CK trial orbit differs from original independent circle");
         }
         if (std::abs(u-2)>1e-3) ++nonfinal;
      }
      require(trials.size()>=4 && nonfinal>=2, "CK witness lacks distinguishable actual trial attitudes");
      write(output+"/trials.json",QJsonDocument(trials).toJson());
      erract_c("SET",0,const_cast<char *>("RETURN"));
      furnsh_c(FileManager::Instance()->GetFullPathname("LEAP_SECS_FILE").c_str()); spiceCheck();
      SpiceDouble et0; str2et_c("2000 JAN 01 12:00:00 UTC",&et0); spiceCheck();
      const double tt0=unitim_c(et0,"ET","TDT"); spiceCheck();
      SpiceInt handle; dafopr_c((output+"/solution.bc").toUtf8().constData(),&handle); spiceCheck(); dafbfs_c(handle);
      SpiceBoolean found; daffna_c(&found); spiceCheck(); QJsonArray segments,knots;
      double maximumMatrix=0, maximumClock=0;
      while (found) {
         SpiceDouble summary[125], endpoints[2]; SpiceInt integers[6]; dafgs_c(summary); dafus_c(summary,2,6,endpoints,integers); spiceCheck();
         require(integers[0]==-100057000 && integers[1]==1 && integers[2]==5 && integers[3]==0 && integers[4]>0 && integers[5]>=integers[4] && integers[5]-integers[4]<10000, "CK segment identity/frame/type/layout differs");
         std::vector<double> words(integers[5]-integers[4]+1); dafgda_c(handle,integers[4],integers[5],words.data()); spiceCheck();
         require(words.size()>=6, "CK segment is too short");
         const int n=static_cast<int>(words.back()), intervals=static_cast<int>(words[words.size()-2]);
         require(n==25 && intervals==1 && words[words.size()-4]==1 && words[words.size()-3]==8 && words.size()==5*n+(n-1)/100+intervals+(intervals-1)/100+5, "CK final-pass packet count or interpolation layout differs");
         require(endpoints[0]==words[4*n] && endpoints[1]==words[5*n-1], "CK descriptor does not bound its packets");
         for (int i=0;i<n;++i) {
            SpiceDouble matrix[3][3]; q2m_c(words.data()+4*i,matrix); spiceCheck();
            QJsonArray quaternion,observedMatrix;
            for (int j=0;j<4;++j) quaternion.append(words[4*i+j]);
            const auto expected=fixture["expected_matrix"].toArray();
            for (int j=0;j<9;++j) {
               const double error=std::abs(matrix[j/3][j%3]-expected[j].toDouble()); maximumMatrix=std::max(maximumMatrix,error);
               require(error<fixture["matrix_tolerance"].toDouble(), "CK contains a trial attitude or wrong final rotation"); observedMatrix.append(matrix[j/3][j%3]);
            }
            SpiceDouble epoch; sct2e_c(-100057,words[4*n+i],&epoch); spiceCheck();
            const double expectedEt=unitim_c(tt0+5*i,"TDT","ET"); spiceCheck();
            maximumClock=std::max(maximumClock,std::abs(epoch-expectedEt));
            require(std::abs(epoch-expectedEt)<fixture["epoch_tolerance_seconds"].toDouble(), "CK packet epoch differs from independent final schedule");
            if (i) require(words[4*n+i]>words[4*n+i-1], "CK final epochs are not strictly increasing");
            knots.append(QJsonObject{{"elapsed",5*i},{"tick",words[4*n+i]},{"et",epoch},{"expected_et",expectedEt},{"quaternion",quaternion},{"matrix",observedMatrix}});
         }
         segments.append(QJsonObject{{"instrument",integers[0]},{"frame",integers[1]},{"type",integers[2]},{"subtype",1},{"packets",n},{"first",endpoints[0]},{"last",endpoints[1]}});
         daffna_c(&found); spiceCheck();
      }
      dafcls_c(handle); spiceCheck(); require(segments.size()==1 && knots.size()==25, "CK contains extra trial segments");
      const auto proof=QJsonObject{{"status","passed"},{"case",spec["key"]},{"solver_type",spec["solver_type"]},{"solver_converged",true},{"final_control",solver->GetSolverVariable(0)},
         {"trial_rows",trials.size()},{"nonfinal_trial_rows",nonfinal},{"max_trial_state_error",trialError},{"segments",segments},{"knots",knots},
         {"max_matrix_error",maximumMatrix},{"max_epoch_error_seconds",maximumClock},{"new_requirement_credit",0},
         {"scope","Native-script Quaternion CK final-pass membership; frozen constant Euler attitude distinguishes trial controls. No new GUI Quaternion selector, whole E406, terminal policy or full-build qualification."}};
      write(output+"/proof.json",QJsonDocument(proof).toJson());
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n'; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; }
   return 1;
}
