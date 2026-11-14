#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "PropagationGroupsDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

using Matrix=std::array<std::array<double,6>,6>;
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static Matrix identity() {Matrix result{};for (int i=0;i<6;++i) result[i][i]=1;return result;}
static Matrix multiply(const Matrix &a,const Matrix &b)
{
   Matrix result{};for (int i=0;i<6;++i) for (int j=0;j<6;++j) for (int k=0;k<6;++k) result[i][j]+=a[i][k]*b[k][j];return result;
}
// Exact variational solution about a circular Kepler orbit. The Hill-frame
// solution is transformed at both ends; it is not an inertial STM by itself.
static Matrix analyticSTM(double n,double time)
{
   const double u=n*time,c=std::cos(u),s=std::sin(u);
   Matrix hill{};
   hill[0]={4-3*c,0,0,s/n,2*(1-c)/n,0};
   hill[1]={6*(s-u),1,0,-2*(1-c)/n,(4*s-3*u)/n,0};
   hill[2]={0,0,c,0,0,s/n};
   hill[3]={3*n*s,0,0,c,2*s,0};
   hill[4]={-6*n*(1-c),0,0,-2*s,4*c-3,0};
   hill[5]={0,0,-n*s,0,0,c};
   auto initial=identity();initial[3][1]=n;initial[4][0]=-n;
   Matrix final{};const double rotation[3][3]={{c,-s,0},{s,c,0},{0,0,1}};
   const double omega[3][3]={{0,-n,0},{n,0,0},{0,0,0}};
   for (int i=0;i<3;++i) for (int j=0;j<3;++j) {
      final[i][j]=final[i+3][j+3]=rotation[i][j];
      for (int k=0;k<3;++k) final[i+3][j]+=rotation[i][k]*omega[k][j];
   }
   return multiply(final,multiply(hill,initial));
}
static double symplecticError(const Matrix &phi)
{
   Matrix form{},transpose{};for (int i=0;i<3;++i) {form[i][i+3]=1;form[i+3][i]=-1;}
   for (int i=0;i<6;++i) for (int j=0;j<6;++j) transpose[i][j]=phi[j][i];
   const auto value=multiply(transpose,multiply(form,phi));double error=0;
   for (int i=0;i<6;++i) for (int j=0;j<6;++j) error=std::max(error,std::abs(value[i][j]-form[i][j]));return error;
}
static QList<double> numbers(const QString &path,int count)
{
   QFile file(path);require(file.open(QIODevice::ReadOnly),"Variational report missing");const auto words=QString::fromUtf8(file.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==count,"Variational report has an unexpected number of components");QList<double> values;
   for (const auto &word:words) {bool valid=false;const auto value=word.toDouble(&valid);require(valid && std::isfinite(value),"Variational report has nonfinite/nonnumeric data");values.append(value);}return values;
}
static Matrix reportedSTM(const QString &path)
{
   const auto values=numbers(path,36);Matrix result{};for (int i=0;i<6;++i) for (int j=0;j<6;++j) result[i][j]=values[6*i+j];return result;
}
static GmatCommand *propagation()
{
   auto *command=Moderator::Instance()->GetFirstCommand();while (command && command->GetTypeName()!="Propagate") command=command->GetNext();require(command,"Runtime propagation command missing");return command;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaStateTransition");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup, direction and supported GUI platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Variational fixture directory missing");
      const QString direction=QString::fromLocal8Bit(argv[2]);require(direction=="forward" || direction=="backward","Unknown variational direction");const bool backward=direction=="backward";
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      constexpr double mu=398600.4418,radius=7000;const double speed=std::sqrt(mu/radius),n=std::sqrt(mu/(radius*radius*radius)),duration=backward ? -600 : 600;
      const auto zero=analyticSTM(n,0);for (int i=0;i<6;++i) for (int j=0;j<6;++j) require(std::abs(zero[i][j]-(i==j ? 1.0 : 0.0))<1e-14,"Independent STM oracle does not satisfy its initial condition");
      const auto stateFile=files.filePath("state.txt"),stmFile=files.filePath("stm.txt"),clockFile=files.filePath("clock.txt");
      const QString command="Propagate 'Variational arc' "+QString(backward ? "BackProp " : "")+"Prop(Sat) {Sat.ElapsedSecs = "+QString::number(duration)+", StopTolerance = 1e-9}; % preserve variational comment";
      const QString enabledCommand=QString(command).replace("Prop(Sat)","Prop(Sat, 'STM')");
      const QString mission="BeginMissionSequence;\n"+command+"\nReport States Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\nReport Matrices Sat.OrbitSTM;\nReport Clocks Sat.ElapsedSecs;\n";
      const QString source="% Preserve variational source Δ\nEarth.Mu = 398600.4418;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2024 12:00:00.000';\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(speed,'g',17)+";\nSat.VZ = 0;\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag.AtmosphereModel = None;\nGravity.SRP = Off;\nGravity.RelativisticCorrection = Off;\nCreate Propagator Prop;\nProp.FM = Gravity;\nProp.Accuracy = 1e-13;\nProp.InitialStepSize = 10;\nProp.MaxStep = 30;\nCreate ReportFile States Matrices;\nStates.Filename = '"+stateFile+"';\nStates.WriteHeaders = false;\nStates.Precision = 17;\nMatrices.Filename = '"+stmFile+"';\nMatrices.WriteHeaders = false;\nMatrices.Precision = 17;\nCreate Variable Keep;\nKeep = 13; % preserve unrelated source\n"+mission;
      auto fixture=source;fixture.replace("Create ReportFile States Matrices;","Create ReportFile States Matrices Clocks;");fixture.replace("Create Variable Keep;","Clocks.Filename = '"+clockFile+"';\nClocks.WriteHeaders = false;\nClocks.Precision = 17;\nCreate Variable Keep;");
      MainWindow window;require(window.initialize(startup),"Variational runtime initialization failed");window.show();auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(editor && tree && area,"Variational workspace missing");
      editor->setPlainText(fixture);require(window.buildScript(),"Variational fixture did not build");
      auto cartesian=[&](double elapsed) {const double angle=n*elapsed;return std::array<double,6>{radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0};};
      double positionError=0,velocityError=0,achievedTime=0,reportedTime=0,maxStopError=0;Matrix actual{};
      auto run=[&](bool enabled) {
         events();auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");require(messages,"Variational diagnostics window missing");messages->clear();
         if (window.runMission()!=MainWindow::RunResult::Completed) {events();std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<std::endl;throw std::runtime_error("Variational mission failed");}
         auto *setup=dynamic_cast<PropSetup *>(propagation()->GetClone(0));require(setup && setup->GetPropagator(),"Running variational integrator clone missing");achievedTime=setup->GetPropagator()->GetTime();reportedTime=numbers(clockFile,1).front();
         // Double-precision epoch arithmetic can miss a requested sub-microsecond
         // stop. Isolate the dynamics oracle from that separate timing limit.
         require(std::abs(achievedTime-duration)<=1e-6 && std::abs(reportedTime-duration)<=1e-6,"Variational arc missed its requested duration by more than one microsecond");maxStopError=std::max(maxStopError,std::abs(reportedTime-duration));
         if (std::abs(reportedTime-duration)>1e-9) {
            events();const auto text=messages->toPlainText();
            const auto tolerance=QRegularExpression("stopping tolerance \\(([-+0-9.eE]+)\\)").match(text);
            const auto difference=QRegularExpression("difference from the desired value is ([-+0-9.eE]+)").match(text);
            require(text.contains("Variational arc") && text.contains("achieved stop is outside") && tolerance.hasMatch() && tolerance.captured(1).toDouble()==1e-9 && difference.hasMatch() && std::abs(std::abs(difference.captured(1).toDouble())-std::abs(reportedTime-duration))<=1e-12,"Documented time-quantization warning missing or has wrong tolerance/achieved difference");
         }
         const auto state=numbers(stateFile,6);actual=reportedSTM(stmFile);const auto expected=cartesian(achievedTime);
         for (int i=0;i<6;++i) {const double error=std::abs(state[i]-expected[i]);if (i<3) positionError=std::max(positionError,error);else velocityError=std::max(velocityError,error);if (error>(i<3 ? 1e-6 : 1e-9)) {std::cerr.precision(17);std::cerr<<"Cartesian component "<<i<<" enabled="<<enabled<<" actual="<<state[i]<<" expected="<<expected[i]<<" error="<<error<<std::endl;}require(error<=(i<3 ? 1e-6 : 1e-9),"Cartesian state differs from the independent circular-orbit solution");}
         auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(sat,"Running variational spacecraft missing");const auto &runtime=sat->GetRmatrixParameter("OrbitSTM");require(runtime.GetNumRows()>=6 && runtime.GetNumColumns()>=6,"Runtime orbit STM has fewer than 36 components");
         for (int i=0;i<6;++i) for (int j=0;j<6;++j) {require(std::abs(actual[i][j]-runtime(i,j))<=1e-13*std::max(1.0,std::abs(runtime(i,j))),"STM report differs from running spacecraft matrix");if (!enabled) require(std::abs(actual[i][j]-(i==j ? 1.0 : 0.0))<=1e-14,"Disabled STM propagated or retained stale values");}
         require(propagation()->GetBooleanParameter("AllSTMs")==enabled,"Running propagation STM option differs from GUI selection");return state;
      };
      const auto offState=run(false);const auto offTime=achievedTime;
      CommandEditor *panel=nullptr;QMdiSubWindow *child=nullptr;
      auto open=[&] {
         const auto snapshot=window.missionSnapshot();int index=-1;for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="Propagate") index=i;require(index>=0,"Variational mission node missing");
         QTreeWidgetItemIterator item(tree);while (*item && (!(*item)->data(0,Qt::UserRole).isValid() || (*item)->data(0,Qt::UserRole).toInt()!=index)) ++item;require(*item,"Variational mission tree item missing");
         for (auto *page=static_cast<QWidget *>(tree);page;page=page->parentWidget()) for (auto *tabs:window.findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
         tree->setCurrentItem(*item);tree->scrollToItem(*item);events();require(tree->isVisible() && tree->isEnabled(),"Variational Mission tree is not visible/enabled");acceptanceView(window);tree->itemDoubleClicked(*item,0);events();
         // A minimized outer test desktop need not activate the newly opened
         // inner window. Identify the real editor by its mission-node identity,
         // then select it in the workspace before inspecting its controls.
         QList<QMdiSubWindow *> matches;for (auto *candidate:area->subWindowList()) if (candidate->property("commandIndex").isValid() && candidate->property("commandIndex").toInt()==index && dynamic_cast<CommandEditor *>(candidate->widget())) matches.append(candidate);
         require(matches.size()==1,"Actual variational command editor missing, duplicated or belongs to the wrong command");child=matches.front();panel=dynamic_cast<CommandEditor *>(child->widget());child->showNormal();area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);require(child->isVisible() && panel->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==snapshot.nodes[index].statement,"Selected variational editor is hidden or contains a different command");
      };
      auto groups=[&](bool enabled,bool accept,bool checkInvalid) {
         auto *button=panel->findChild<QPushButton *>("editPropagationGroups");require(button && button->isVisible() && button->isEnabled(),"Propagation assignments action missing");std::exception_ptr failure;bool visited=false;
         QTimer::singleShot(0,panel,[&] {
            auto *dialog=panel->findChild<QDialog *>("propagationGroupsDialog");
            try {
               require(dialog,"Variational assignment dialog did not open");acceptanceView(*dialog);visited=true;
               auto *stm=dialog->findChild<QCheckBox *>("propagationGroupSTM");auto *cov=dialog->findChild<QCheckBox *>("propagationGroupCovariance");auto *amat=dialog->findChild<QCheckBox *>("propagationGroupAMatrix");auto *table=dialog->findChild<QTableWidget *>("propagationGroupsTable");auto *box=dialog->findChild<QDialogButtonBox *>();
               require(stm && stm->isVisible() && stm->isEnabled() && cov && !cov->isChecked() && amat && !amat->isChecked() && table && table->rowCount()==1 && table->item(0,1)->text()=="Sat" && box,"Variational flags/group state differs from applied command");
               if (checkInvalid) {table->item(0,1)->setText("");events();require(!box->button(QDialogButtonBox::Ok)->isEnabled() && !dialog->findChild<QLabel *>("propagationGroupsError")->text().isEmpty(),"Empty Cartesian propagation group accepted or lacks diagnostic");table->item(0,1)->setText("Sat");events();}
               stm->setChecked(enabled);require(box->button(QDialogButtonBox::Ok)->isEnabled(),"Valid variational assignment disables OK");box->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            } catch (...) {failure=std::current_exception();if (dialog) dialog->reject();}
         });button->click();if (failure) std::rethrow_exception(failure);require(visited,"Variational assignment dialog not exercised");
      };
      open();const auto undo=editor->document()->availableUndoSteps();groups(true,false,true);
      require(!panel->hasChanges() && editor->toPlainText()==fixture && editor->document()->availableUndoSteps()==undo && !propagation()->GetBooleanParameter("AllSTMs"),"STM Cancel changed command/source/model/history");
      groups(true,true,false);auto *pending=panel->findChild<QPlainTextEdit *>("commandSource");require(pending && pending->toPlainText()==enabledCommand && panel->hasChanges() && editor->toPlainText()==fixture && !propagation()->GetBooleanParameter("AllSTMs"),"STM OK bypassed pending Apply or changed unrelated command text");
      pending->undo();require(pending->toPlainText()==command,"Pending STM Undo was not exact");pending->redo();require(pending->toPlainText()==enabledCommand,"Pending STM Redo was not exact");
      panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CommandEditor *>(child->widget());const auto enabledSource=QString(fixture).replace(command,enabledCommand);require(panel && !panel->hasChanges() && editor->toPlainText()==enabledSource && propagation()->GetBooleanParameter("AllSTMs"),"STM Apply did not commit the exact command/model");
      require(child->close(),"Clean variational command editor did not close");events();editor->undo();require(editor->toPlainText()==fixture && window.buildScript() && !propagation()->GetBooleanParameter("AllSTMs"),"Applied STM Undo did not restore exact source/model");editor->redo();require(editor->toPlainText()==enabledSource && window.buildScript() && propagation()->GetBooleanParameter("AllSTMs"),"Applied STM Redo did not restore exact source/model");
      const auto saved=files.filePath("Variational "+direction+" Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==enabledSource && propagation()->GetBooleanParameter("AllSTMs"),"STM Unicode save/reopen changed exact source/model");
      const auto onState=run(true);const auto onTime=achievedTime,onReportedTime=reportedTime;const auto expected=analyticSTM(n,onTime);std::array<double,4> blockError{};double nonidentity=0;
      for (int i=0;i<6;++i) for (int j=0;j<6;++j) {
         const auto error=std::abs(actual[i][j]-expected[i][j]);const int block=2*(i/3)+(j/3);blockError[block]=std::max(blockError[block],error);const double tolerance=block==1 ? 2e-6 : block==2 ? 2e-11 : 2e-8;
         if (error>tolerance) std::cerr<<"STM["<<i<<","<<j<<"] actual "<<actual[i][j]<<" expected "<<expected[i][j]<<" error "<<error<<std::endl;
         require(error<=tolerance,"A propagated STM entry differs from the independent circular-orbit variational solution");nonidentity=std::max(nonidentity,std::abs(actual[i][j]-(i==j ? 1.0 : 0.0)));
      }
      const double canonicalError=symplecticError(actual);require(canonicalError<=2e-8 && nonidentity>1,"STM does not preserve the canonical form or did not evolve");
      const auto onTruth=cartesian(onTime),offTruth=cartesian(offTime);
      for (int i=0;i<6;++i) require(std::abs((onState[i]-offState[i])-(onTruth[i]-offTruth[i]))<=(i<3 ? 1e-6 : 1e-9),"Enabling STM changed the nominal Cartesian trajectory after accounting for achieved times");
      open();groups(false,true,false);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CommandEditor *>(child->widget());require(panel && !panel->hasChanges() && editor->toPlainText()==fixture && !propagation()->GetBooleanParameter("AllSTMs"),"GUI STM disable did not restore the exact original command/model");require(child->close(),"Disabled variational command editor did not close");events();run(false);
      QJsonArray errors;for (double value:blockError) errors.append(value);
      const QJsonObject proof{{"checkpoint","variational."+direction},{"direction",direction},{"duration_seconds",duration},{"quantities",QJsonArray{"CartesianState","OrbitSTM"}},{"stm_entries_checked",36},{"outcome","passed"},
         {"facets",QJsonArray{"actual-mission-editor","empty-group-rejection","stm-cancel","pending-apply","pending-and-applied-exact-undo-redo","unicode-save-reopen","runtime-flag-and-matrix","six-state-circular-oracle","36-entry-independent-variational-oracle","symplectic-form","stm-on-off-state-fidelity","disabled-identity-no-stale-matrix","documented-clock-quantization-warning"}},
         {"max_cartesian_position_error_km",positionError},{"max_cartesian_velocity_error_km_per_s",velocityError},{"max_stm_block_errors_rr_rv_vr_vv",errors},{"symplectic_error",canonicalError},
         {"oracle_integration_duration_seconds",onTime},{"reported_elapsed_seconds",onReportedTime},{"requested_stop_tolerance_seconds",1e-9},{"max_reported_stop_error_seconds",maxStopError},{"requested_stop_tolerance_met",maxStopError<=1e-9},
         {"scope","one point-mass circular equatorial orbit; not general perturbed STM, covariance, frame, multi-group or ephemeris qualification"}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
     catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
