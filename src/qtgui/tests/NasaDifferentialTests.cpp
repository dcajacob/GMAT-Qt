#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "Moderator.hpp"
#include "GmatGlobal.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
using Pair=std::array<double,2>;
using Row=std::array<double,4>;
static void require(bool value,const char *message) {if (!value) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void watch(QWidget &widget)
{
   acceptanceView(widget);
   if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {QEventLoop loop;QTimer::singleShot(std::max(120,qEnvironmentVariableIntValue("GMAT_QT_ACCEPTANCE_PAUSE_MS")),&loop,&QEventLoop::quit);loop.exec();}
}
static void reveal(QWidget &control)
{
   for (auto *page=&control;page;page=page->parentWidget()) {
      for (auto *tabs:control.window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(&control);
   }
   events();require(control.isVisible() && control.isEnabled(),qPrintable("Solver control not visible/enabled: "+control.objectName()));
}
static QLineEdit *entry(QWidget &owner,const QString &field)
{
   auto *value=owner.findChild<QLineEdit *>("resource_"+field);require(value,qPrintable("Solver entry missing: "+field));reveal(*value);return value;
}
static void select(QWidget &owner,const QString &field,const QString &value,int count=0)
{
   auto *choice=owner.findChild<QComboBox *>("resource_"+field);require(choice && !choice->isEditable() && choice->findText(value)>=0 && (!count || choice->count()==count),qPrintable("Typed solver selector missing: "+field));reveal(*choice);choice->setCurrentText(value);events();require(choice->currentText()==value,"Solver selector did not retain requested value");
}
static void modal(const std::function<void()> &trigger,const std::function<void(QDialog &)> &inspect)
{
   bool visited=false;std::exception_ptr failure;QTimer::singleShot(0,[&] {auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());try {require(dialog,"Solver modal missing");watch(*dialog);visited=true;inspect(*dialog);}catch (...) {failure=std::current_exception();if (dialog) dialog->reject();}});trigger();if (failure) std::rethrow_exception(failure);require(visited,"Solver modal not inspected");
}
static void reportPicker(QWidget &owner,const QString &path,bool accept)
{
   auto *value=entry(owner,"ReportFile");const auto original=value->text();const auto buttons=value->parentWidget()->findChildren<QPushButton *>();require(buttons.size()==1,"Displayed solver report picker missing");reveal(*buttons.front());require(!QFileInfo::exists(path),"Report path already exists before selection");
   modal([&]{buttons.front()->click();},[&](QDialog &dialog) {auto *picker=dynamic_cast<QFileDialog *>(&dialog);require(picker && picker->acceptMode()==QFileDialog::AcceptSave && picker->fileMode()==QFileDialog::AnyFile && picker->testOption(QFileDialog::DontConfirmOverwrite),"Solver report picker is not a non-writing output chooser");if (!accept) {picker->reject();return;}auto *filename=picker->findChild<QLineEdit *>("fileNameEdit");require(filename && filename->isVisible(),"Report picker filename entry missing");filename->setText(path);events();auto *save=picker->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save);require(save && save->isEnabled(),"Report filename Save disabled");save->click();require(picker->result()==QDialog::Accepted,"Report picker did not accept path");});require(value->text()==(accept ? path : original) && !QFileInfo::exists(path),"Report picker wrote a file or changed Cancel state");
}
static QMap<QString,QString> properties()
{
   auto *dc=Moderator::Instance()->GetConfiguredObject("DC");require(dc && dc->IsOfType("DifferentialCorrector"),"Configured differential corrector missing");QMap<QString,QString> values;for (const auto &field:resourceProperties(*dc)) values[field.name]=field.value;return values;
}
static QString read(const QString &path) {QFile file(path);require(file.open(QIODevice::ReadOnly),qPrintable("Solver artifact missing: "+path));return QString::fromUtf8(file.readAll());}
static QJsonArray json(const Pair &p) {return {p[0],p[1]};}
static QJsonArray json(const Row &r) {return {r[0],r[1],r[2],r[3]};}
static std::vector<Row> rows(const QString &text)
{
   std::vector<Row> values;for (const auto &line:text.trimmed().split('\n',Qt::SkipEmptyParts)) {const auto tokens=line.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(tokens.size()==4,"Trial report must have four columns");Row row{};for (int i=0;i<4;++i) {bool ok=false;row[i]=tokens[i].toDouble(&ok);require(ok && std::isfinite(row[i]),"Trial report value is nonnumeric/nonfinite");}values.push_back(row);}return values;
}
static Pair correction(double x,double y,int sign)
{
   const double a=2*x+sign*0.02,det=2*a-3,e1=11-x*x-y,e2=13-3*x-2*y;
   return {x+(2*e1-e2)/det,y+(a*e2-3*e1)/det};
}
static void near(double actual,double expected,double tolerance,const char *message)
{
   if (!std::isfinite(actual) || std::abs(actual-expected)>tolerance) {std::cerr<<message<<" actual="<<actual<<" expected="<<expected<<" error="<<std::abs(actual-expected)<<std::endl;throw std::runtime_error(message);}
}
static const QString number="([-+]?(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)";
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaDifferential");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup, DC case and supported GUI platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Differential acceptance temporary directory missing");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const QString name=QString::fromLocal8Bit(argv[2]);
      const QString prefix=name.startsWith("Forward") ? "Forward" : name.startsWith("Central") ? "Central" : name.startsWith("Backward") ? "Backward" : "";require(!prefix.isEmpty(),"Unknown DC derivative case");const auto suffix=name.mid(prefix.size());require(QStringList{"Verbose","Normal","Concise","Debug","Quiet","Limit1","Limit2","Converged1"}.contains(suffix),"Unknown DC case");
      const QString method=prefix+"Difference",style=QStringList{"Normal","Concise","Debug"}.contains(suffix) ? suffix : "Verbose";const bool show=suffix!="Quiet",initialSolved=suffix=="Converged1",expectedConverged=!suffix.startsWith("Limit");const int maximum=suffix=="Limit1" || initialSolved ? 1 : suffix=="Limit2" ? 2 : 30,sign=prefix=="Forward" ? 1 : prefix=="Backward" ? -1 : 0;
      const auto createdReport=files.filePath("Created solver "+name+" Δ.txt"),report=files.filePath("Applied solver "+name+" Δ.txt"),trials=files.filePath("trials.txt"),final=files.filePath("final.txt");
      const QString keep="Keep = 13; % preserve initializer Δ\n",simple="BeginMissionSequence;\nKeep = Keep + 1; % preserve simple mission\n";
      const QString baseline="% Differential-corrector acceptance Δ\nCreate Variable X Y F1 F2 Keep;\n"+keep+"Create Spacecraft Anchor;\nAnchor.X = 7300;\nAnchor.Y = 0;\nAnchor.Z = 0;\nAnchor.VX = 0;\nAnchor.VY = 7.4;\nAnchor.VZ = 0;\nCreate ReportFile Trials Final;\nTrials.Filename = '"+trials+"';\nTrials.Precision = 17;\nTrials.WriteHeaders = false;\nTrials.SolverIterations = All;\nFinal.Filename = '"+final+"';\nFinal.Precision = 17;\nFinal.WriteHeaders = false;\n"+simple;
      MainWindow window;require(window.initialize(startup),"Differential acceptance initialization failed");window.show();auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");require(editor && tree && area && messages,"Differential workspace missing");
      auto build=[&] {if (!window.buildScript()) {std::cerr<<messages->toPlainText().toStdString()<<std::endl;throw std::runtime_error("Differential fixture build failed");}};editor->setPlainText(baseline);build();const auto initialUndo=editor->document()->availableUndoSteps();
      auto unchanged=[&] {require(editor->toPlainText()==baseline && editor->document()->availableUndoSteps()==initialUndo && !Moderator::Instance()->GetConfiguredObject("DC"),"DC draft changed source/model/history");};
      const QString initialMethod=prefix=="Forward" ? "BackwardDifference" : "ForwardDifference",initialStyle=style=="Normal" ? "Concise" : "Normal";
      int invalidCount=0;
      auto create=[&](bool accept) {auto *action=window.findChild<QAction *>("createResource");require(action && action->isEnabled(),"New DC action missing");modal([&]{action->trigger();},[&](QDialog &dialog) {
         auto *types=dialog.findChild<QComboBox *>("resourceType");auto *newName=dialog.findChild<QLineEdit *>("resourceName");require(types && newName && types->findText("DifferentialCorrector")>=0,"New DC controls missing");newName->setText("DC");types->setCurrentText("DifferentialCorrector");events();select(dialog,"Algorithm","NewtonRaphson",3);select(dialog,"DerivativeMethod",initialMethod,3);select(dialog,"ReportStyle",initialStyle,4);select(dialog,"ShowProgress",show ? "false" : "true",2);entry(dialog,"MaximumIterations")->setText(QString::number(maximum+1));reportPicker(dialog,createdReport,false);reportPicker(dialog,createdReport,true);unchanged();watch(dialog);
         auto *buttons=dialog.findChild<QDialogButtonBox *>("resourceCreationButtons");require(buttons,"New DC buttons missing");if (accept && name=="ForwardVerbose") {auto *value=entry(dialog,"MaximumIterations");for (const auto &bad:QStringList{"0","-1","1.5","nan","inf","","not-an-integer","2147483648"}) {value->setText(bad);buttons->button(QDialogButtonBox::Ok)->click();events();++invalidCount;require(dialog.isVisible() && value->text()==bad && !dialog.findChild<QLabel *>("resourceCreationStatus")->text().isEmpty(),"Invalid DC Create accepted or lost pending input/diagnostic");unchanged();watch(dialog);}value->setText(QString::number(maximum+1));}
         buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();if (accept && dialog.result()!=QDialog::Accepted) std::cerr<<dialog.findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;require(dialog.result()==(accept ? QDialog::Accepted : QDialog::Rejected),"Valid DC Create/Cancel failed");});};
      create(false);unchanged();create(true);events();const auto created=editor->toPlainText();const auto createdValues=properties();require(created.endsWith(simple) && created.contains(keep),"DC creation rewrote unrelated source");
      ResourceEditor *panel=nullptr;QMdiSubWindow *child=nullptr;auto open=[&](bool trigger) {if (trigger) {const auto items=tree->findItems("DC",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"DC tree entry missing");tree->itemDoubleClicked(items.front(),0);events();}QList<QMdiSubWindow *> matches;for (auto *candidate:area->subWindowList()) if (candidate->property("resourceName")=="DC" && dynamic_cast<ResourceEditor *>(candidate->widget())) matches.append(candidate);require(matches.size()==1,"Actual DC resource editor missing or duplicated");child=matches.front();panel=dynamic_cast<ResourceEditor *>(child->widget());child->showNormal();area->setActiveSubWindow(child);child->raise();events();watch(*child);require(!panel->hasChanges(),"Fresh DC panel has pending edits");};
      open(false);require(child->close(),"Clean created DC panel did not close");events();editor->undo();require(editor->toPlainText()==baseline,"DC creation Undo not exact");build();require(!Moderator::Instance()->GetConfiguredObject("DC"),"DC creation Undo retained object");editor->redo();require(editor->toPlainText()==created,"DC creation Redo not exact");build();require(properties()==createdValues,"DC creation Redo changed properties");
      int previewChecks=0;
      auto preview=[&](const QMap<QString,QString> &wanted) {
         const auto before=editor->toPlainText();const auto beforeValues=properties();const auto beforeUndo=editor->document()->availableUndoSteps();auto *button=panel->findChild<QPushButton *>("showScript");require(button,"DC Show script action missing");reveal(*button);
         modal([&]{button->click();},[&](QDialog &dialog) {auto *text=dialog.findChild<QPlainTextEdit *>("inspectionText");require(dialog.objectName()=="inspectionDialog" && dialog.windowTitle()=="Resource script" && text && text->isReadOnly(),"DC Show script is not the actual read-only resource inspector");const auto displayed=text->toPlainText();require(displayed.contains("Create DifferentialCorrector DC;"),"DC script preview omitted resource declaration");for (auto it=wanted.cbegin();it!=wanted.cend();++it) {const auto pattern="DC\\."+QRegularExpression::escape(it.key())+"\\s*=\\s*'?"+QRegularExpression::escape(it.value())+"'?\\s*;";require(QRegularExpression(pattern).match(displayed).hasMatch(),qPrintable("DC preview does not show applied "+it.key()));}watch(dialog);dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Close)->click();});
         ++previewChecks;require(editor->toPlainText()==before && properties()==beforeValues && editor->document()->availableUndoSteps()==beforeUndo,"DC Show script changed source/model/history");
      };
      open(true);const auto editUndo=editor->document()->availableUndoSteps();select(*panel,"DerivativeMethod",method,3);select(*panel,"ReportStyle",style,4);select(*panel,"ShowProgress",show ? "true" : "false",2);reportPicker(*panel,report,false);reportPicker(*panel,report,true);entry(*panel,"MaximumIterations")->setText("0");if (name=="ForwardVerbose") preview({{"Algorithm","NewtonRaphson"},{"DerivativeMethod",initialMethod},{"ReportStyle",initialStyle},{"ShowProgress",show ? "false" : "true"},{"MaximumIterations",QString::number(maximum+1)},{"ReportFile",createdReport}});panel->requestApply();events();++invalidCount;require(panel->hasChanges() && entry(*panel,"MaximumIterations")->text()=="0" && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && editor->toPlainText()==created && properties()==createdValues && editor->document()->availableUndoSteps()==editUndo,"Invalid DC Apply changed source/model/history or lost pending input/diagnostic");entry(*panel,"MaximumIterations")->setText(QString::number(maximum));watch(*child);panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Corrected DC Apply failed");const auto applied=editor->toPlainText();const auto appliedValues=properties();if (name=="ForwardVerbose") preview({{"Algorithm","NewtonRaphson"},{"DerivativeMethod",method},{"ReportStyle",style},{"ShowProgress",show ? "true" : "false"},{"MaximumIterations",QString::number(maximum)},{"ReportFile",report}});
      require(child->close(),"Applied DC panel did not close");events();editor->undo();require(editor->toPlainText()==created,"DC Apply Undo not exact");build();require(properties()==createdValues,"DC Apply Undo changed original properties");editor->redo();require(editor->toPlainText()==applied,"DC Apply Redo not exact");build();require(properties()==appliedValues,"DC Apply Redo changed properties");
      const QString mission="BeginMissionSequence;\nTarget 'analytic coupled target' DC {SolveMode = Solve, ExitMode = SaveAndContinue, ShowProgressWindow = true}; % retain target label\nVary DC(X = "+QString(initialSolved ? "3" : "2")+", {Perturbation = 0.02, Lower = 0.1, Upper = 10, MaxStep = 10});\nVary DC(Y = "+QString(initialSolved ? "2" : "1")+", {Perturbation = 0.04, Lower = -10, Upper = 10, MaxStep = 10});\nF1 = X^2 + Y;\nF2 = 3*X + 2*Y;\nReport Trials X Y F1 F2;\nAchieve DC(F1 = 11, {Tolerance = 0.000000001});\nAchieve DC(F2 = 13, {Tolerance = 0.000000001});\nEndTarget; % preserve target end\nKeep = Keep + 1; % prove continuation\nReport Final X Y F1 F2;\n";
      require(applied.endsWith(simple),"DC settings changed fixture mission");editor->setPlainText(applied.left(applied.size()-simple.size())+mission);build();const auto source=editor->toPlainText();const auto saved=files.filePath("DC "+name+" Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved),"DC Unicode save/reopen failed");build();require(editor->toPlainText()==source && properties()==appliedValues && read(saved)==source,"DC persistence changed exact source/properties");
      auto *configured=Moderator::Instance()->GetConfiguredObject("DC");require(configured->GetStringParameter("DerivativeMethod")==method.toStdString() && configured->GetStringParameter("ReportStyle")==style.toStdString() && configured->GetBooleanParameter("ShowProgress")==show && configured->GetIntegerParameter("MaximumIterations")==maximum && configured->GetStringParameter("ReportFile")==report.toStdString(),"DC persisted settings differ from requested values");require(!QFileInfo::exists(report) && !QFileInfo::exists(createdReport),"GUI settings wrote solver report before execution");messages->clear();const auto runResult=window.runMission();events();const auto message=messages->toPlainText();
      // Preserve raw evidence even if an assertion below identifies a defect.
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!output.isEmpty()) {QDir dest(output);for (const auto &item:QList<QPair<QString,QString>>{{"source.script",source},{"messages.txt",message},{"trials.txt",QFileInfo::exists(trials) ? read(trials) : ""},{"final.txt",QFileInfo::exists(final) ? read(final) : ""},{"solver.txt",QFileInfo::exists(report) ? read(report) : ""}}) {QFile file(dest.filePath(item.first));require(file.open(QIODevice::WriteOnly) && file.write(item.second.toUtf8())==item.second.toUtf8().size(),"Could not retain solver evidence");}}
      if (runResult!=MainWindow::RunResult::Completed) std::cerr<<message.toStdString()<<std::endl;require(runResult==MainWindow::RunResult::Completed,"DC mission did not continue after targeting");auto *keepValue=Moderator::Instance()->GetInternalObject("Keep");require(keepValue && keepValue->GetRealParameter("Value")==14,"Target did not continue to known post-loop mission result");require(GmatGlobal::Instance()->GetSolverStatus("DC")== (expectedConverged ? Gmat::CONVERGED : Gmat::EXCEEDED_ITERATIONS),"DC run status differs from analytic convergence/iteration limit");
      const auto trialRows=rows(read(trials));const auto finalRows=rows(read(final));require(finalRows.size()==1 && !trialRows.empty(),"DC final/trial report lacks data");QJsonArray trialProof;for (const auto &r:trialRows) {near(r[2],r[0]*r[0]+r[1],5e-10,"Trial nonlinear goal differs from independent function");near(r[3],3*r[0]+2*r[1],5e-10,"Trial linear goal differs from independent function");trialProof.append(json(r));}
      std::vector<Row> nominal;std::size_t index=0;Pair expected=initialSolved ? Pair{3,2} : Pair{2,1};const int perturbations=sign==0 ? 4 : 2;
      while (true) {require(index<trialRows.size(),"DC trial report omitted nominal pass");const auto r=trialRows[index++];near(r[0],expected[0],5e-11,"Nominal X differs from independent finite-difference Newton update");near(r[1],expected[1],5e-11,"Nominal Y differs from independent finite-difference Newton update");nominal.push_back(r);const bool converged=std::abs(r[2]-11)<=1e-9 && std::abs(r[3]-13)<=1e-9;if (converged || int(nominal.size())==maximum) {require(converged==expectedConverged,"Analytic stopping condition differs from requested case");break;}
         const std::vector<Pair> steps=sign==0 ? std::vector<Pair>{{0.02,0},{-0.02,0},{0,0.04},{0,-0.04}} : std::vector<Pair>{{sign*0.02,0},{0,sign*0.04}};
         for (const auto &step:steps) {require(index<trialRows.size(),"DC omitted requested perturbation pass");const auto pert=trialRows[index++];near(pert[0],r[0]+step[0],5e-11,"DC X perturbation direction/value differs");near(pert[1],r[1]+step[1],5e-11,"DC Y perturbation direction/value differs");}expected=correction(r[0],r[1],sign);
      }
      require(index+1==trialRows.size() && trialRows.size()==nominal.size()+(nominal.size()-1)*perturbations+1,"DC ran extra/stale or omitted iterations/perturbations/accepted pass");for (int i=0;i<4;++i) near(trialRows[index][i],nominal.back()[i],5e-11,"Final publish pass did not retain last nominal state");for (int i=0;i<4;++i) near(finalRows.front()[i],nominal.back()[i],5e-11,"DC final state did not retain last nominal pass");if (expectedConverged) {near(finalRows.front()[0],3,1e-8,"DC did not reach independent positive root X=3");near(finalRows.front()[1],2,1e-8,"DC did not reach independent positive root Y=2");}
      QString solver;QJsonArray matrixProof;const QString iterationPattern="(?:^|\\n)Iteration ([0-9]+)\\nRunning Nominal Pass\\nVariables:";const QRegularExpression iterations(iterationPattern);const QRegularExpression messageIterations("DC Iteration ([0-9]+); Nominal Pass");
      if (show) {
         require(QFileInfo::exists(report),"ShowProgress true omitted configured report");solver=read(report);auto it=iterations.globalMatch(solver);std::vector<QRegularExpressionMatch> blocks;while (it.hasNext()) blocks.push_back(it.next());require(blocks.size()==nominal.size(),"Solver report omitted an iteration");require(solver.count("Jacobian (Sensitivity matrix):")==int(nominal.size())-1 && solver.count("Inverse Jacobian:")==int(nominal.size())-1,"Solver report has missing/stale Jacobian matrices");
         auto parseNumber=[&](const QString &text,const QString &pattern,int group=1) {const auto match=QRegularExpression(pattern).match(text);require(match.hasMatch(),qPrintable("Solver progress/report field missing: "+pattern));bool ok=false;const double value=match.captured(group).toDouble(&ok);require(ok && std::isfinite(value),"Solver progress/report number is invalid");return value;};
         for (std::size_t i=0;i<blocks.size();++i) {require(blocks[i].captured(1).toInt()==int(i+1),"Report iteration numbers are not consecutive");const auto block=solver.mid(blocks[i].capturedStart(),(i+1<blocks.size() ? blocks[i+1].capturedStart() : solver.size())-blocks[i].capturedStart());const auto r=nominal[i];near(parseNumber(block,"(?:^|\\n)\\s*X = "+number),r[0],5e-11,"Report control X differs from actual nominal");near(parseNumber(block,"(?:^|\\n)\\s*Y = "+number),r[1],5e-11,"Report control Y differs from actual nominal");
            for (int goal=0;goal<2;++goal) {const QString label="F"+QString::number(goal+1);const auto match=QRegularExpression(label+"\\s+Desired: "+number+"\\s+Achieved: "+number).match(block);require(match.hasMatch(),"Report goal/achieved pair missing");near(match.captured(1).toDouble(),goal==0 ? 11 : 13,1e-12,"Report desired goal differs");near(match.captured(2).toDouble(),r[goal+2],5e-10,"Report achieved goal differs from nominal");}
            if (i+1<blocks.size()) {const auto matrix=QRegularExpression("Jacobian \\(Sensitivity matrix\\):\\s+"+number+"\\s+"+number+"\\s+"+number+"\\s+"+number).match(block);require(matrix.hasMatch(),"Solver report omitted Jacobian");const double a=2*r[0]+sign*0.02,det=2*a-3;const Row expectedJac{a,3,1,2},expectedInverse{2/det,-3/det,-1/det,a/det};const auto inverse=QRegularExpression("Inverse Jacobian:\\s+"+number+"\\s+"+number+"\\s+"+number+"\\s+"+number).match(block);require(inverse.hasMatch(),"Solver report omitted inverse Jacobian");Row actualJac{},actualInverse{};for (int j=0;j<4;++j) {actualJac[j]=matrix.captured(j+1).toDouble();actualInverse[j]=inverse.captured(j+1).toDouble();near(actualJac[j],expectedJac[j],1e-10,"Reported Jacobian differs from analytic finite differences");near(actualInverse[j],expectedInverse[j],1e-10,"Reported inverse Jacobian differs from analytic inverse");}matrixProof.append(QJsonObject{{"iteration",int(i+1)},{"jacobian",json(actualJac)},{"inverse",json(actualInverse)}});}
         }
         require(solver.contains("Perturbing with variable values:")==(nominal.size()>1 && (style=="Verbose" || style=="Debug")),"ReportStyle perturbation details do not follow intended level");if (style=="Debug" && nominal.size()>1) require(solver.contains("Command stream data:") && solver.contains("Anchor.X") && solver.contains("7300"),"Debug report omitted controlled-object script details");else require(!solver.contains("Command stream data:"),"Non-Debug report includes Debug snippets");
         auto itMsg=messageIterations.globalMatch(message);std::vector<QRegularExpressionMatch> msgBlocks;while (itMsg.hasNext()) msgBlocks.push_back(itMsg.next());require(msgBlocks.size()==nominal.size(),"Message window omitted nominal progress iterations");for (std::size_t i=0;i<msgBlocks.size();++i) {require(msgBlocks[i].captured(1).toInt()==int(i+1),"Message progress iteration numbers not consecutive");const auto block=message.mid(msgBlocks[i].capturedStart(),(i+1<msgBlocks.size() ? msgBlocks[i+1].capturedStart() : message.size())-msgBlocks[i].capturedStart());const auto r=nominal[i];near(parseNumber(block,"X = "+number),r[0],5e-10,"Message control X differs");near(parseNumber(block,"Y = "+number),r[1],5e-10,"Message control Y differs");for (int goal=0;goal<2;++goal) {const auto match=QRegularExpression("F"+QString::number(goal+1)+"\\s+Desired: "+number+"\\s+Achieved: "+number+"\\s+Variance: "+number).match(block);require(match.hasMatch(),"Message goal/achieved/variance missing");const double target=goal==0 ? 11 : 13;near(match.captured(1).toDouble(),target,1e-12,"Message desired target differs");near(match.captured(2).toDouble(),r[goal+2],5e-10,"Message achieved goal differs");near(match.captured(3).toDouble(),target-r[goal+2],5e-10,"Message goal variance differs");}}
      }else require(!QFileInfo::exists(report) && !message.contains("Performing Differential Correction") && !messageIterations.match(message).hasMatch() && !message.contains("Goals and achieved values:"),"ShowProgress false still emitted solver progress/report");
      if (expectedConverged) require(!message.contains("has exceeded the maximum number of allowed iterations") && !message.contains("Targeter did NOT converge"),"Converged boundary falsely reports iteration-limit failure");else require(message.contains("has exceeded the maximum number of allowed iterations") && message.contains("Targeter did NOT converge"),"Iteration-limited solve omitted nonconvergence diagnostic");
      auto *table=window.findChild<QTableWidget *>("solverProgress");auto *status=window.findChild<QLabel *>("solverStatus");require(table && table->rowCount()==4 && status && status->property("converged").isValid() && status->property("converged").toBool()==expectedConverged,"Solver status window omitted/staled convergence or rows");for (int row=0;row<table->rowCount();++row) {const auto label=table->item(row,1)->text();const int i=label=="X" ? 0 : label=="Y" ? 1 : label=="F1" ? 2 : label=="F2" ? 3 : -1;require(i>=0,"Solver window unexpected row name");near(table->item(row,2)->text().toDouble(),nominal.back()[i],5e-10,"Solver window current value differs from last nominal");if (i>=2) {near(table->item(row,3)->text().toDouble(),i==2 ? 11 : 13,1e-12,"Solver window desired value differs");near(table->item(row,4)->text().toDouble(),nominal.back()[i]-(i==2 ? 11 : 13),5e-10,"Solver window residual differs");}}
      if (auto *solverChild=qobject_cast<QMdiSubWindow *>(table->parentWidget()->parentWidget())) {solverChild->showNormal();area->setActiveSubWindow(solverChild);solverChild->raise();watch(*solverChild);}if (auto *dock=qobject_cast<QDockWidget *>(messages->parentWidget())) dock->show();watch(*messages);
      QJsonArray facets{"actual-new-and-resource-editors","typed-derivative-style-boolean-controls","nonwriting-output-picker-cancel-select","create-cancel-isolation","invalid-input-correction-atomicity","exact-creation-edit-undo-redo","unicode-save-reopen","runtime-convergence-and-continuation","independent-coupled-nonlinear-root","independent-per-iteration-perturbations-newton-updates","checked-message-and-report-progress-policy","checked-live-solver-table-status"};
      if (show) {facets.append("checked-message-report-controls-goals-iterations");facets.append(nominal.size()>1 ? "checked-jacobian-and-inverse" : "no-stale-matrices-without-updates");}else facets.append("no-message-or-report-when-disabled");
      if (previewChecks) facets.append("applied-script-preview-pending-isolation-and-refresh");
      QJsonArray nominalProof;for (const auto &r:nominal) nominalProof.append(json(r));const QJsonObject proof{{"checkpoint","differential."+name},{"case",name},{"method",method},{"report_style",style},{"show_progress",show},{"maximum_iterations",maximum},{"expected_converged",expectedConverged},{"actual_converged",GmatGlobal::Instance()->GetSolverStatus("DC")==Gmat::CONVERGED},{"initially_solved",initialSolved},{"nominal_iterations",int(nominal.size())},{"perturbations_per_update",perturbations},{"final_publish_passes",1},{"trial_rows",trialProof},{"nominal_rows",nominalProof},{"matrix_records",matrixProof},{"final_values",json(finalRows.front())},{"invalid_rejections_checked",invalidCount},{"script_preview_checks",previewChecks},{"messages",message},{"solver_report",solver},{"outcome","passed"},
         {"facets",facets},
         {"scope","Specified two-variable polynomial target, unconstrained interior Newton updates and selected iteration boundaries. Not general nonlinear/ill-conditioned/scaled/bounded targeting, Broyden variants, propagation targeting, native-input/platform/stress or full historical validation domains."}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
