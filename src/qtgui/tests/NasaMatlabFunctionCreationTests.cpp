#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "MatlabInterface.hpp"
#include "InterfaceException.hpp"
#include <QAction>
#include <QComboBox>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <vector>
using namespace formaccept;

static void reveal(QWidget *widget) {
 require(widget,"MATLAB function control missing");
 for(auto *p=widget;p;p=p->parentWidget()) {
  for(auto *tabs:widget->window()->findChildren<QTabWidget*>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
  if(auto *scroll=qobject_cast<QScrollArea*>(p))scroll->ensureWidgetVisible(widget);
 }
 events();require(widget->isVisible()&&widget->isEnabled(),"MATLAB function control hidden/disabled");acceptanceView(*widget);
}

static QString isolatedStartup(const QString &original,const QString &directory,const QString &plugin) {
 const QDir installation(QFileInfo(original).absolutePath());
 QStringList lines=QString::fromUtf8(read(original)).split('\n');
 const QRegularExpression assignment("^(\\w+)\\s*=\\s*(.*)$");
 const QMap<QString,QString> settings{{"OUTPUT_PATH",directory+"/output/"},{"LOG_FILE",directory+"/output/GmatLog.txt"},{"PERSONALIZATION_FILE",directory+"/MyGmat.ini"},{"MATLAB_MODE","SHARED"}};
 QSet<QString> changed;
 for(auto &line:lines) {
  const auto match=assignment.match(line);if(!match.hasMatch())continue;
  const auto key=match.captured(1);auto value=match.captured(2).trimmed();
  if(settings.contains(key)){line=key+" = "+settings[key];changed.insert(key);}
  else if(value.startsWith("../")||value.startsWith("./")) {
   const bool slash=value.endsWith('/');value=QDir::cleanPath(installation.absoluteFilePath(value));if(slash)value+='/';line=key+" = "+value;
  }
 }
 for(auto i=settings.begin();i!=settings.end();++i)if(!changed.contains(i.key()))lines<<i.key()+" = "+i.value();
 bool present=false;for(const auto &line:lines)if(line.startsWith("PLUGIN")&&line.contains("libMatlabInterface"))present=true;
 if(!present)lines<<"PLUGIN = "+plugin;
 require(QDir().mkpath(directory+"/output"),"MATLAB private output directory failed");
 const auto path=directory+"/startup.txt";write(path,lines.join('\n').toUtf8());return path;
}

int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaMatlabFunctionCreation");
 try {
  require(argc==3,"Supply startup and MATLAB plugin");QTemporaryDir runtime;require(runtime.isValid(),"MATLAB runtime directory missing");
  const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
  const auto directory=runtime.filePath("MATLAB functions Ω");require(QDir().mkpath(directory),"MATLAB function directory failed");
  const auto function=directory+"/MatProbe.m";
  const QByteArray body="function [total, difference] = MatProbe(a, b)\ntotal = a + b;\ndifference = a - b;\nend\n";
  write(function,body);Rig rig(startup);rig.save("MatProbe.m",body);rig.save("startup.txt",read(startup));
  const QString mission="BeginMissionSequence;\nReport Values Keep;\n";
  const QString seed="Create Variable Input Other Total Difference Keep;\nInput = 7;\nOther = -3;\nTotal = 0;\nDifference = 0;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n"+mission;
  rig.source->setPlainText(seed);require(rig.window.buildScript(),"MATLAB creation seed failed");rig.save("seed.script",seed.toUtf8());
  QTreeWidgetItem *category=nullptr;
  for(QTreeWidgetItemIterator it(rig.resources);*it;++it)if((*it)->data(0,Qt::UserRole+1).isValid()&&(*it)->data(0,Qt::UserRole+1).toUInt()==Gmat::FUNCTION&&(*it)->data(0,Qt::UserRole).toString().isEmpty()){require(!category,"Duplicate function category");category=*it;}
  require(category,"Actual function Resource Tree category absent");
  QJsonArray creation;
  auto create=[&](bool commit) {
   rig.resources->expandAll();rig.resources->scrollToItem(category);rig.resources->setCurrentItem(category);reveal(rig.resources);
   const int undo=rig.source->document()->availableUndoSteps();bool menuSeen=false,dialogSeen=false;std::exception_ptr failure;QTimer timer;timer.setInterval(20);
   QObject::connect(&timer,&QTimer::timeout,&rig.window,[&] {
    try {
     if(!menuSeen) {
      auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)return;
      auto *action=menu->findChild<QAction*>("addResource_MatlabFunction");require(action&&action->isEnabled(),"Actual Resource Tree Add MATLAB function absent");reveal(menu);menuSeen=true;QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;
     }
     auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!dialog||dialog->objectName()!="newResourceDialog")return;
     timer.stop();dialogSeen=true;reveal(dialog);auto *types=dialog->findChild<QComboBox*>("resourceType");auto *name=dialog->findChild<QLineEdit*>("resourceName");auto *buttons=dialog->findChild<QDialogButtonBox*>("resourceCreationButtons");
     require(types&&types->count()==1&&types->currentText()=="MatlabFunction"&&name&&buttons,"MATLAB creator type/controls differ");reveal(name);name->setFocus();QTest::keyClick(name,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(name,"MatProbe");events();
     require(rig.source->toPlainText()==seed&&rig.source->document()->availableUndoSteps()==undo&&!Moderator::Instance()->GetConfiguredObject("MatProbe"),"Pending MATLAB creation changed caller/native object/history");
     QTest::mouseClick(buttons->button(commit?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);
     require(dialog->result()==(commit?QDialog::Accepted:QDialog::Rejected),"MATLAB creator decision differs");
    }catch(...){failure=std::current_exception();timer.stop();if(auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget()))dialog->reject();if(auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget()))menu->close();}
   });
   timer.start();const auto point=rig.resources->visualItemRect(category).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,rig.resources->viewport()->mapToGlobal(point));QApplication::sendEvent(rig.resources->viewport(),&event);timer.stop();if(failure)std::rethrow_exception(failure);require(menuSeen&&dialogSeen,"Actual MATLAB creation route not reached");events();creation.append(QJsonObject{{"accepted",commit},{"menu",menuSeen},{"creator",dialogSeen}});
  };
  create(false);require(rig.source->toPlainText()==seed&&!Moderator::Instance()->GetConfiguredObject("MatProbe"),"MATLAB creator Cancel changed source/model");create(true);
  auto *object=Moderator::Instance()->GetConfiguredObject("MatProbe");require(object&&object->GetTypeName()=="MatlabFunction","Creator did not register actual MatlabFunction");
  const auto created=rig.source->toPlainText();rig.closePanels();
  auto entries=rig.resources->findItems("MatProbe",Qt::MatchExactly|Qt::MatchRecursive);require(entries.size()==1,"Created MATLAB tree resource missing");rig.resources->itemDoubleClicked(entries.first(),0);events();
  auto *owner=rig.area->activeSubWindow();auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr;require(panel,"Actual MATLAB resource editor absent");owner->showNormal();owner->move(0,0);owner->resize(rig.area->viewport()->size());
  auto *path=panel->findChild<QLineEdit*>("resource_FunctionPath");reveal(path);require(!path->isReadOnly(),"MATLAB FunctionPath is not editable");const auto oldPath=object->GetStringParameter("FunctionPath");
  path->setFocus();QTest::keyClick(path,Qt::Key_A,Qt::ControlModifier);QApplication::clipboard()->setText(function);QTest::keyClick(path,Qt::Key_V,Qt::ControlModifier);events();
  require(path->text()==function&&panel->hasChanges()&&rig.source->toPlainText()==created&&object->GetStringParameter("FunctionPath")==oldPath,"MATLAB path draft escaped isolation");
  auto *buttons=panel->findChild<QDialogButtonBox*>("resourceButtons");require(buttons,"MATLAB Apply buttons missing");reveal(buttons->button(QDialogButtonBox::Apply));QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();
  require(Moderator::Instance()->GetConfiguredObject("MatProbe")->GetStringParameter("FunctionPath")==function.toStdString(),"MATLAB path Apply/native value differs");
  rig.closePanels();const auto applied=rig.source->toPlainText();require(applied!=created&&applied.endsWith(mission),"MATLAB Apply changed following mission or omitted path");
  rig.source->undo();require(rig.source->toPlainText()==created&&rig.window.buildScript(),"MATLAB path Undo differs");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"MATLAB path Redo differs");rig.save("applied.script",applied.toUtf8());
  QJsonArray parameters;object=Moderator::Instance()->GetConfiguredObject("MatProbe");
  for(int id=0;id<object->GetParameterCount();++id)parameters.append(QJsonObject{{"field",QString::fromStdString(object->GetParameterText(id))},{"type",QString::fromStdString(object->GetParameterTypeString(id))},{"read_only",object->IsParameterReadOnly(id)}});
  const QString caller=applied.left(applied.size()-mission.size())+"BeginMissionSequence;\n[Total, Difference] = MatProbe(Input, Other);\nReport Values Total Difference Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
  rig.source->setPlainText(caller);require(rig.window.buildScript(),"MATLAB caller Build failed");const auto saved=rig.files.filePath("MATLAB caller Ω.script");require(rig.window.saveScriptTo(saved),"MATLAB caller Save failed");rig.save("caller.script",caller.toUtf8());QJsonArray phases;
  for(const auto &phase:QStringList{"initial","reopened"}) {
   if(phase=="reopened")require(rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==caller,"MATLAB caller Unicode reopen differs");
   const int undo=rig.source->document()->availableUndoSteps();const auto status=rig.window.runMission();events();rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"Actual GMAT-to-MATLAB call failed");
   QJsonArray values;for(const auto &pair:std::vector<std::pair<std::string,double>>{{"Input",7},{"Other",-3},{"Total",4},{"Difference",10},{"Keep",42}}) {
    auto *native=Moderator::Instance()->GetInternalObject(pair.first);require(native&&native->GetTypeName()=="Variable"&&native->GetRealParameter("Value")==pair.second,"Independent MATLAB result/caller value differs");values.append(native->GetRealParameter("Value"));
   }
   const auto bytes=read(rig.report);rig.save(phase+"-report.csv",bytes);const auto lines=QString::fromUtf8(bytes).trimmed().split('\n');require(lines.size()==2,"MATLAB report row count differs");
   const auto first=lines[0].split(',');require(first.size()==3&&first[0].toDouble()==4&&first[1].toDouble()==10&&first[2].toDouble()==41&&lines[1].trimmed().toDouble()==42,"MATLAB raw report differs from independent values");
   auto *output=rig.window.findChild<QTreeWidget*>("Output");require(output,"MATLAB Output Tree absent");const auto reports=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive);require(reports.size()==1,"MATLAB actual report output absent");output->itemDoubleClicked(reports.first(),0);events();QMdiSubWindow *viewer=nullptr;
   for(auto *candidate:rig.area->subWindowList())if(dynamic_cast<ReportViewer*>(candidate->widget())){require(!viewer,"Ambiguous MATLAB report viewer");viewer=candidate;}
   require(viewer,"MATLAB report viewer absent");rig.area->setActiveSubWindow(viewer);auto *shown=viewer->findChild<QPlainTextEdit*>();require(shown&&shown->isReadOnly()&&shown->toPlainText().trimmed()==QString::fromUtf8(bytes).trimmed(),"Actual MATLAB report viewer differs from raw bytes");acceptanceView(*viewer);require(viewer->close(),"MATLAB report viewer did not close");events();rig.closePanels();
   require(rig.source->toPlainText()==caller&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==caller.toUtf8()&&read(function)==body,"MATLAB Run changed caller/history/file");
   auto *prototype=Moderator::Instance()->GetConfiguredObject("MatProbe");require(prototype&&prototype->GetTypeName()=="MatlabFunction"&&prototype->GetStringParameter("FunctionPath")==function.toStdString(),"MATLAB function prototype changed after Run");
   phases.append(QJsonObject{{"phase",phase},{"native_values",values},{"report",QString::fromUtf8(bytes)},{"viewer_matches",true}});
  }
  auto *matlab=MatlabInterface::Instance();require(matlab&&matlab->IsOpen()&&Moderator::Instance()->GetMatlabInterface()==matlab,"Actual command/interface engine identity differs");
  matlab->RunMatlabString("gmat_acceptance_recovery = 314159;");
  bool rejected=false;QString error;
  try {matlab->RunMatlabString("error('GMAT:Acceptance','Expected MATLAB acceptance failure');");}
  catch(InterfaceException &e){error=QString::fromStdString(e.GetFullMessage());rejected=error.contains("Expected MATLAB acceptance failure");}
  require(rejected,"Expected MATLAB evaluation error was not propagated");
  bool recovered=false;QString recoveryError;
  try {matlab->RunMatlabString("gmat_acceptance_recovery = gmat_acceptance_recovery + 1;");recovered=true;}
  catch(InterfaceException &e){recoveryError=QString::fromStdString(e.GetFullMessage());}
  Real value=-1;Integer rows=0,columns=0;const auto count=matlab->GetRealArray("gmat_acceptance_recovery",1,&value,rows,columns);
  QJsonObject recovery{{"intentional_error_rejected",rejected},{"intentional_error",error},{"following_eval_succeeded",recovered},{"following_eval_error",recoveryError},{"following_value",value},{"rows",rows},{"columns",columns},{"elements",count}};
  rig.save("error-recovery.json",QJsonDocument(recovery).toJson());
  require(recovered&&count==1&&rows==1&&columns==1&&value==314160,"MATLAB error state contaminated following successful evaluation");
  bool syntaxRejected=false;
  try {matlab->RunMatlabString("gmat_invalid_syntax = ;");}
  catch(InterfaceException &){syntaxRejected=true;}
  require(syntaxRejected,"MATLAB syntax error was silently accepted");
  matlab->RunMatlabString("gmat_acceptance_recovery = gmat_acceptance_recovery + 1;");
  require(matlab->GetRealArray("gmat_acceptance_recovery",1,&value,rows,columns)==1&&value==314161&&rows==1&&columns==1,"MATLAB workspace or recovery after syntax error differs");
  recovery["syntax_error_rejected"]=syntaxRejected;recovery["post_syntax_value"]=value;rig.save("error-recovery.json",QJsonDocument(recovery).toJson());
  matlab->RunMatlabString("gmat_eval_status_0 = 9;");
  require(matlab->GetRealArray("gmat_eval_status_0",1,&value,rows,columns)==1&&value==9,"MATLAB scratch-like output name was overwritten");
  matlab->RunMatlabString("gmat_eval_status_0 = gmat_eval_status_0 + 1;");
  require(matlab->GetRealArray("gmat_eval_status_0",1,&value,rows,columns)==1&&value==10,"MATLAB scratch-like input/output name was overwritten");
  recovery["scratch_named_expression_value"]=value;rig.save("error-recovery.json",QJsonDocument(recovery).toJson());
  const QJsonObject proof{{"checkpoint","matlab-function-resource-creation"},{"outcome","passed"},{"creation",creation},{"native_type","MatlabFunction"},{"native_function_path",function},{"parameter_metadata",parameters},{"function_source",QString::fromUtf8(body)},{"phases",phases},{"error_recovery",recovery},{"original_key","All Others!E152"},{"new_requirement_credit",0},{"limits","Resource Tree MATLAB-function creation/configuration and prescribed two-scalar call only; broader MATLAB I/O, process sharing/isolation, optimizer and reverse-interface scopes remain open."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
