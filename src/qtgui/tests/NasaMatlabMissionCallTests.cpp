#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include "CommandEditor.hpp"
#include "ReportParameterDialog.hpp"
#include "ReportViewer.hpp"
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QTabWidget>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <QtTest/QTest>
using namespace formaccept;

static QString quote(QString s) { return "'"+s.replace("'","''")+"'"; }
static void reveal(QWidget *widget) {
 require(widget,"MATLAB Mission Tree control absent");
 for(auto *p=widget;p;p=p->parentWidget()) {
  if(auto *scroll=qobject_cast<QScrollArea*>(p))scroll->ensureWidgetVisible(widget);
  if(auto *tabs=qobject_cast<QTabWidget*>(p))
   for(auto *page=widget;page&&page!=tabs;page=page->parentWidget())
    if(tabs->indexOf(page)>=0)tabs->setCurrentWidget(page);
 }
 events();acceptanceView(*widget);require(widget->isVisible()&&widget->isEnabled(),"MATLAB Mission Tree control unavailable");
}
struct Context {QString body,action,marker;int calls=1;int keep=42;int reportedKeep=42;bool reportBefore=false;};
static Context context(const QString &mode) {
 const QString report="Report 'Witness' Values Input Other Copy Result Keep;\n";
 Context c;c.action="Append inside branch…";
 if(mode=="Beginning") {c.body="Report 'Anchor' Values Input Other Copy Result Keep;\nKeep = Keep + 1;\n";c.action="Insert before…";c.marker="Report 'Anchor'";c.reportedKeep=41;}
 else if(mode=="Middle"||mode=="End") {c.body="Keep = Keep + 1;\nReport 'Anchor' Values Input Other Copy Result Keep;\n";c.action=mode=="End"?"Append command…":"Insert before…";c.marker=mode=="End"?QString():"Report 'Anchor'";c.reportBefore=mode=="End";}
 else if(mode=="After") {c.body="Keep = Keep + 1;\nReport 'Anchor' Values Input Other Copy Result Keep;\n";c.action="Insert after…";c.marker="% Preserve mission tail Ω";c.reportBefore=true;}
 else if(mode=="AfterBranch") {c.body="If 'Anchor' Input > 0;\nKeep = Keep + 1;\nEndIf;\n"+report;c.action="Insert after…";c.marker="Report 'Witness'";}
 else if(mode=="If") {c.body="If 'Anchor' Input > 0;\nKeep = Keep + 1;\nElse;\nKeep = Keep + 100;\nEndIf;\n"+report;c.marker="Else;";}
 else if(mode=="Else") {c.body="If 'Anchor' Input < 0;\nKeep = Keep + 100;\nElse;\nKeep = Keep + 1;\nEndIf;\n"+report;c.action="Insert before…";c.marker="EndIf;";}
 else if(mode=="For"||mode=="Nested") {c.body="For 'Anchor' Index = 1:1:2;\nKeep = Keep + 1;\nEndFor;\n";if(mode=="Nested")c.body="If 'Outer' Input > 0;\n"+c.body+"EndIf;\n";c.body+=report;c.marker="EndFor;";c.calls=2;c.keep=c.reportedKeep=43;}
 else if(mode=="While") {c.body="While 'Anchor' Keep < 43;\nKeep = Keep + 1;\nEndWhile;\n"+report;c.marker="EndWhile;";c.calls=2;c.keep=c.reportedKeep=43;}
 else if(mode=="Target") {c.body="Target 'Anchor' DC {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\nVary DC(Control = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 1});\nKeep = Keep + 1;\nAchieve DC(Control = 1, {Tolerance = 0.000001});\nEndTarget;\n"+report;c.marker="EndTarget;";c.calls=2;c.keep=c.reportedKeep=43;}
 else if(mode=="Optimize") {c.body="Optimize 'Anchor' Opt {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\nVary Opt(Control = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 1});\nKeep = Keep + 1;\nObjective = Control^2;\nMinimize Opt(Objective);\nEndOptimize;\n"+report;c.marker="EndOptimize;";c.calls=2;c.keep=c.reportedKeep=43;}
 else throw std::runtime_error("Unknown MATLAB Mission Tree context");
 return c;
}
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaMatlabMissionCall");
 try {
  require(argc==4,"Supply startup, MATLAB plugin and mission context");const QString mode=argv[3];const auto c=context(mode);
  QTemporaryDir runtime;require(runtime.isValid(),"MATLAB Mission Tree startup directory missing");const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),runtime.path(),QFileInfo(argv[2]).absoluteFilePath());
  const auto validator=QFileInfo(QFileInfo(argv[1]).dir().filePath("GmatQt")).canonicalFilePath();require(QFileInfo(validator).isExecutable(),"MATLAB Mission Tree validator missing");FileManager::Instance()->SetBinDirectory("GmatQt",validator.toStdString());
  Rig rig(startup);rig.save("startup.txt",read(startup));rig.save("validator-path.txt",validator.toUtf8());
  const auto directory=rig.files.filePath("MATLAB Mission Ω");require(QDir().mkpath(directory),"MATLAB Mission Tree function directory missing");const auto alpha=directory+"/MatAlpha.m",beta=directory+"/MatBeta.m",calls=directory+"/calls.csv";
  const QByteArray alphaBody="function [a,b] = MatAlpha(x,y)\na = 2*x-y;\nb = x+3*y;\nend\n";
  const auto betaBody=("function [a,b] = MatBeta(x,y)\nf = fopen("+quote(calls)+", 'a');\nassert(f >= 0);\nfprintf(f, '%.17g,%.17g\\n', x, y);\nfclose(f);\na = x-4*y;\nb = 2*y+x;\nend\n").toUtf8();write(alpha,alphaBody);write(beta,betaBody);rig.save("MatAlpha.m",alphaBody);rig.save("MatBeta.m",betaBody);
  const QString declarations="% MATLAB Mission Tree fixture Ω\nCreate MatlabFunction MatAlpha MatBeta;\nMatAlpha.FunctionPath = "+quote(alpha)+";\nMatBeta.FunctionPath = "+quote(beta)+";\nCreate Variable Input Other Copy Result Keep Index Control Objective;\nInput = 7;\nOther = -3;\nCopy = 0;\nResult = 0;\nKeep = 41;\nControl = 1;\nCreate DifferentialCorrector DC;\nCreate Yukon Opt;\nCreate ReportFile Values;\nValues.Filename = "+quote(rig.report)+";\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\n";
  const auto seed=declarations+c.body+"% Preserve mission tail Ω\n";const auto saved=rig.files.filePath("MATLAB Mission caller Ω.script");rig.source->setPlainText(seed);const auto built=rig.window.buildScript();rig.save("seed-build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built)std::cerr<<rig.messages->toPlainText().toStdString();require(built&&rig.window.saveScriptTo(saved),"MATLAB Mission Tree seed Build/Save failed");rig.save("seed.script",seed.toUtf8());
  auto anchor=[&] {
   const auto snap=rig.window.missionSnapshot();int index=-1;
   for(int i=0;i<snap.nodes.size();++i)if(mode=="Else"?snap.nodes[i].type=="EndIf":missionCommandLabel(snap.nodes[i].statement).name=="Anchor"){require(index<0,"Ambiguous MATLAB Mission Tree anchor");index=i;}
   require(index>=0,"MATLAB Mission Tree anchor missing");QTreeWidgetItem *found=nullptr;for(QTreeWidgetItemIterator it(rig.mission);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)found=*it;require(found,"MATLAB Mission Tree anchor item missing");return found;
  };
  int openings=0;
  auto open=[&] {
   auto *item=anchor();rig.mission->expandAll();rig.mission->setCurrentItem(item);rig.mission->scrollToItem(item);reveal(rig.mission);std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,&rig.window,[&] {try{auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());require(menu,"MATLAB Mission Tree insertion popup missing");QAction *choice=nullptr;for(auto *a:menu->actions())if(a->text()==c.action)choice=a;require(choice&&choice->isEnabled(),"MATLAB Mission Tree insertion action unavailable");menu->setActiveAction(choice);reveal(menu);visited=true;QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(choice).center());}catch(...){failure=std::current_exception();if(auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget()))menu->close();}});
   rig.mission->customContextMenuRequested(rig.mission->visualItemRect(item).center());events();if(failure)std::rethrow_exception(failure);require(visited,"MATLAB Mission Tree popup not exercised");auto *child=rig.area->activeSubWindow();require(child&&dynamic_cast<CommandEditor*>(child->widget()),"Actual MATLAB insertion command editor missing");child->showNormal();child->move(0,0);child->resize(rig.area->viewport()->size());rig.area->setActiveSubWindow(child);child->raise();events();reveal(child);
   auto *templates=child->findChild<QComboBox*>("commandTemplate");QJsonArray offered;if(templates)for(int i=1;i<templates->count();++i)offered.append(templates->itemText(i));rig.save("offered-templates.json",QJsonDocument(offered).toJson());
   require(templates&&templates->findText("Call MATLAB function")>=0,"Actual MATLAB Mission Tree insertion template missing");auto *fresh=child->findChild<QPlainTextEdit*>("commandSource");rig.save(QString("fresh-template-%1.json").arg(++openings),QJsonDocument(QJsonObject{{"text",fresh?fresh->toPlainText():QString()},{"modified",fresh&&fresh->document()->isModified()}}).toJson());require(fresh&&fresh->toPlainText().isEmpty()&&!fresh->document()->isModified(),"New MATLAB insertion draft is unexpectedly edited");reveal(templates);templates->showPopup();auto *choices=templates->view();const auto entry=templates->model()->index(templates->findText("Call MATLAB function"),0);choices->scrollTo(entry,QAbstractItemView::PositionAtCenter);events();acceptanceView(*choices->window());const auto rect=choices->visualRect(entry).intersected(choices->viewport()->rect());require(!rect.isEmpty(),"MATLAB template popup item is outside its viewport");QTest::mouseClick(choices->viewport(),Qt::LeftButton,Qt::NoModifier,rect.center());events();
   auto *preview=child->findChild<QPlainTextEdit*>("commandSource");require(preview&&preview->toPlainText()=="[OutputVariable] = MatAlpha(InputVariable);","MATLAB template did not select actual configured MatlabFunction");return child;
  };
  const auto undoBefore=rig.source->document()->availableUndoSteps();auto *cancelled=open();cancelled->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();events();require(rig.source->toPlainText()==seed&&rig.source->document()->availableUndoSteps()==undoBefore&&read(saved)==seed.toUtf8(),"MATLAB insertion Cancel changed source/history/disk");
  auto *child=open();int pickerCancels=0;
  auto function=[&](bool commit) {
   auto *button=child->findChild<QPushButton*>("commandChoose_Function");reveal(button);std::exception_ptr failure;
   QTimer::singleShot(0,child,[&] {auto *dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget());try{require(dialog,"Actual MATLAB function resource chooser missing");auto *combo=dialog->findChild<QComboBox*>();require(combo&&combo->findText("MatAlpha")>=0&&combo->findText("MatBeta")>=0,"MATLAB function chooser omitted actual resources");combo->setCurrentText("MatBeta");reveal(dialog);if(commit)dialog->accept();else{dialog->reject();++pickerCancels;}}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});button->click();if(failure)std::rethrow_exception(failure);
  };
  function(false);require(child->findChild<QLineEdit*>("commandField_Function")->text()=="MatAlpha","MATLAB function chooser Cancel changed pending callee");function(true);
  auto arguments=[&](const QString &field,const QStringList &values,bool commit) {
   auto *button=child->findChild<QPushButton*>("commandChoose_"+field);reveal(button);std::exception_ptr failure;
   QTimer::singleShot(0,child,[&] {auto *dialog=dynamic_cast<ReportParameterDialog*>(child->findChild<QDialog*>("reportParameterDialog"));try{require(dialog,"Actual MATLAB argument picker missing");auto *entry=dialog->findChild<QComboBox*>("reportParameterEntry");auto *selected=dialog->findChild<QListWidget*>("reportSelectedParameters");auto *box=dialog->findChild<QDialogButtonBox*>();require(entry&&selected&&box,"MATLAB argument picker controls missing");dialog->findChild<QPushButton*>("reportParameterRemoveAll")->click();for(const auto &v:values){entry->setEditText(v);dialog->findChild<QPushButton*>("reportAddParameter")->click();}require(selected->count()==values.size(),"MATLAB argument picker lost selection");if(values.size()>1){selected->setCurrentRow(1);dialog->findChild<QPushButton*>("reportParameterUp")->click();require(selected->item(0)->text()==values[1],"MATLAB argument picker Up failed");dialog->findChild<QPushButton*>("reportParameterDown")->click();}for(int i=0;i<values.size();++i)require(selected->item(i)->text()==values[i],"MATLAB argument order differs");reveal(dialog);if(commit){require(box->button(QDialogButtonBox::Ok)->isEnabled(),"Valid MATLAB argument picker selection disabled");box->button(QDialogButtonBox::Ok)->click();}else{dialog->reject();++pickerCancels;}}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});button->click();if(failure)std::rethrow_exception(failure);
  };
  const auto beforePickers=child->findChild<QPlainTextEdit*>("commandSource")->toPlainText();arguments("Inputs",{"Input","Other"},false);arguments("Outputs",{"Result","Copy"},false);require(child->findChild<QPlainTextEdit*>("commandSource")->toPlainText()==beforePickers,"MATLAB argument Cancel changed pending source");arguments("Inputs",{"Other","Input"},true);arguments("Outputs",{"Copy","Result"},true);
  auto *name=child->findChild<QLineEdit*>("commandName");reveal(name);name->setText("MATLAB call Ω");events();const QString call="'MATLAB call Ω' [Copy, Result] = MatBeta(Other, Input);";auto *preview=child->findChild<QPlainTextEdit*>("commandSource");require(preview&&preview->toPlainText()==call&&rig.source->toPlainText()==seed&&read(saved)==seed.toUtf8()&&rig.source->document()->availableUndoSteps()==undoBefore,"MATLAB typed draft escaped isolation or changed exact call syntax");rig.save("draft-call.txt",call.toUtf8());
  QString expected=seed;if(mode=="End")expected+=call+"\n";else{const auto at=expected.indexOf(c.marker);require(at>=0,"Independent MATLAB insertion marker missing");expected.insert(at,call+"\n");}
  auto *buttons=child->findChild<QDialogButtonBox*>();reveal(buttons->button(QDialogButtonBox::Apply));buttons->button(QDialogButtonBox::Apply)->click();events();rig.save("apply-messages.txt",rig.messages->toPlainText().toUtf8());rig.save("applied.script",rig.source->toPlainText().toUtf8());
  auto *panel=dynamic_cast<CommandEditor*>(child->widget());if(panel&&panel->hasChanges())std::cerr<<panel->findChild<QLabel*>("commandStatus")->text().toStdString();require(panel&&!panel->hasChanges()&&rig.source->toPlainText()==expected,"MATLAB Mission Tree Apply differs from independent exact source");const auto appliedUndo=rig.source->document()->availableUndoSteps();child->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();events();require(rig.source->toPlainText()==expected&&rig.source->document()->availableUndoSteps()==appliedUndo,"Second MATLAB Apply duplicated command/history");require(child->close(),"Applied MATLAB command editor did not close");events();
  rig.source->undo();require(rig.source->toPlainText()==seed&&rig.window.buildScript(),"MATLAB insertion Undo did not restore exact source/model");rig.source->redo();require(rig.source->toPlainText()==expected&&rig.window.buildScript(),"MATLAB insertion Redo did not restore exact source/model");
  auto nativeCall=[&] {
   const auto snap=rig.window.missionSnapshot();int count=0;QJsonArray parents,nodes;
   for(const auto &node:snap.nodes)nodes.append(QJsonObject{{"type",node.type},{"statement",node.statement},{"label",node.label},{"editable",node.editable},{"start",double(node.start)},{"end",double(node.end)},{"parent",node.parent}});
   rig.save("native-snapshot.json",QJsonDocument(QJsonObject{{"canonical",snap.canonicalScript},{"authored",snap.sourceScript},{"nodes",nodes}}).toJson());
   for(const auto &node:snap.nodes)if(node.type=="CallMatlabFunction"){require(node.editable&&node.statement.trimmed()==(mode=="End"?"% Preserve mission tail Ω\n"+call:call),"Inserted MATLAB command is not safely mapped to its exact source");for(int p=node.parent;p>=0;p=snap.nodes[p].parent)parents.append(snap.nodes[p].type);++count;}
   require(count==1,"Actual native MATLAB command absent or duplicated");return parents;
  };
  const auto parents=nativeCall();rig.save("native-parent-types.json",QJsonDocument(parents).toJson());
  if(mode=="Middle") {
   const auto snapshot=rig.window.missionSnapshot();int callIndex=-1;for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].type=="CallMatlabFunction")callIndex=i;
   QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(rig.mission);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==callIndex)item=*it;require(item,"Existing MATLAB command tree item missing");rig.mission->scrollToItem(item);reveal(rig.mission);rig.mission->itemDoubleClicked(item,0);events();auto *existing=rig.area->activeSubWindow();require(existing&&dynamic_cast<CommandEditor*>(existing->widget()),"Actual existing MATLAB command editor missing");existing->showNormal();existing->move(0,0);existing->resize(rig.area->viewport()->size());reveal(existing);
   for(const auto &field:QList<QPair<QString,QString>>{{"Function","MatBeta"},{"Inputs","Other, Input"},{"Outputs","[Copy, Result]"}}){auto *control=existing->findChild<QLineEdit*>("commandField_"+field.first);require(control&&control->text()==field.second,"Existing MATLAB call form lost exact configured arguments/callee");}
   auto *label=existing->findChild<QLineEdit*>("commandName");require(label&&label->text()=="MATLAB call Ω","Existing MATLAB call form lost command name");const auto unchangedUndo=rig.source->document()->availableUndoSteps();reveal(label);label->setText("Discarded MATLAB label");require(rig.source->toPlainText()==expected&&rig.source->document()->availableUndoSteps()==unchangedUndo,"Existing MATLAB call draft escaped isolation");existing->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();events();require(rig.source->toPlainText()==expected&&rig.source->document()->availableUndoSteps()==unchangedUndo&&read(saved)==seed.toUtf8(),"Existing MATLAB call Cancel changed caller/history/disk");
  }
  require(rig.window.saveScriptTo(saved)&&read(saved)==expected.toUtf8(),"MATLAB Mission Tree Unicode Save differs");QJsonArray phases;
  for(const auto &phase:QStringList{"initial","no-build-repeat","reopened"}) {
   if(phase=="reopened")require(rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==expected,"MATLAB Mission Tree Unicode reopen differs");write(calls,{});const auto beforeUndo=rig.source->document()->availableUndoSteps();acceptanceView(rig.window);const auto status=rig.window.runMission();events();rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<rig.messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"GUI-created MATLAB call mission failed");
   QJsonObject observed;
   for(const auto &key:QStringList{"Input","Other","Copy","Result","Keep"}){auto *object=Moderator::Instance()->GetInternalObject(key.toStdString());if(object&&object->GetTypeName()=="Variable")observed[key]=object->GetRealParameter("Value");}
   rig.save(phase+"-native-observed.json",QJsonDocument(observed).toJson());if(QFileInfo::exists(rig.report))rig.save(phase+"-report.csv",read(rig.report));if(QFileInfo::exists(calls))rig.save(phase+"-calls.csv",read(calls));
   QJsonArray values;for(const auto &v:std::vector<std::pair<std::string,double>>{{"Input",7},{"Other",-3},{"Copy",-31},{"Result",11},{"Keep",double(c.keep)}}){auto *object=Moderator::Instance()->GetInternalObject(v.first);require(object&&object->GetTypeName()=="Variable"&&object->GetRealParameter("Value")==v.second,"GUI-created MATLAB call differs from independent native results/caller controls");values.append(object->GetRealParameter("Value"));}
   const auto raw=read(rig.report);const auto report=QByteArray("7,-3,")+(c.reportBefore?QByteArray("0,0"):QByteArray("-31,11"))+","+QByteArray::number(c.reportedKeep)+"\n";rig.save(phase+"-report.csv",raw);require(raw==report,"GUI-created MATLAB raw report differs from exact position-aware oracle");const auto actualCalls=read(calls);QByteArray expectedCalls;for(int i=0;i<c.calls;++i)expectedCalls+="-3,7\n";rig.save(phase+"-calls.csv",actualCalls);require(actualCalls==expectedCalls,"Actual MATLAB call count/order/arguments differ");require(nativeCall()==parents,"MATLAB command hierarchy changed during Run");
   auto *output=rig.window.findChild<QTreeWidget*>("Output");const auto reports=output?output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive):QList<QTreeWidgetItem*>();require(reports.size()==1,"MATLAB Mission Tree report output missing");output->itemDoubleClicked(reports.first(),0);events();QMdiSubWindow *viewer=nullptr;for(auto *w:rig.area->subWindowList())if(dynamic_cast<ReportViewer*>(w->widget())){require(!viewer,"Ambiguous MATLAB Mission Tree ReportViewer");viewer=w;}require(viewer,"Actual MATLAB Mission Tree ReportViewer absent");auto *text=viewer->findChild<QPlainTextEdit*>();require(text&&text->isReadOnly()&&text->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"MATLAB Mission Tree ReportViewer differs from raw report");acceptanceView(*viewer);require(viewer->close(),"MATLAB Mission Tree report did not close");events();
   require(rig.source->toPlainText()==expected&&rig.source->document()->availableUndoSteps()==beforeUndo&&read(saved)==expected.toUtf8()&&read(alpha)==alphaBody&&read(beta)==betaBody,"MATLAB mission changed source/history/saved caller/function files");phases.append(QJsonObject{{"phase",phase},{"native_values",values},{"report",QString::fromUtf8(raw)},{"actual_calls",QString::fromUtf8(actualCalls)},{"matlab_calls",c.calls},{"viewer_matches",true}});rig.save("phase-results.json",QJsonDocument(phases).toJson());
  }
  rig.save("final.script",expected.toUtf8());const QJsonObject proof{{"checkpoint","matlab-mission-call"},{"case",mode},{"outcome","passed"},{"insertion_menu",c.action},{"template","Call MATLAB function"},{"actual_command_type","CallMatlabFunction"},{"parent_types",parents},{"picker_cancels",pickerCancels},{"insertion_cancelled",true},{"existing_editor_cancelled",mode=="Middle"},{"exact_source_undo_redo",true},{"idempotent_apply",true},{"phases",phases},{"new_requirement_credit",0}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<'\n';return 1;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
