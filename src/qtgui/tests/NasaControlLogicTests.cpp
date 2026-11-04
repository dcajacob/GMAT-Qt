#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "ConditionDialog.hpp"
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Control logic artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Control logic artifact write failed"); }
static QString number(double value) { return QString::number(value,'g',17); }
static QJsonArray rows(const QByteArray &bytes,int width) {
 QJsonArray result; if (bytes.trimmed().isEmpty()) return result;
 for (const auto &line:QString::fromUtf8(bytes).trimmed().split('\n')) {
  const auto columns=line.split(','); require(columns.size()==width,"Control logic report lost columns"); QJsonArray row;
  for (const auto &column:columns) { bool ok=false; const double value=column.trimmed().toDouble(&ok); require(ok&&std::isfinite(value),"Control logic report contains nonfinite value"); row.append(value); }
  result.append(row);
 }
 return result;
}
static bool compare(double left,const QString &op,double right) {
 if (op=="==") return left==right; if (op=="~=") return left!=right;
 if (op=="<") return left<right; if (op=="<=") return left<=right;
 if (op==">") return left>right; if (op==">=") return left>=right;
 throw std::runtime_error("Unknown control logic relation");
}
static void selectParameter(ReportParameterDialog &dialog,const QString &value) {
 auto *entry=dialog.findChild<QComboBox *>("reportParameterEntry"); auto *box=dialog.findChild<QDialogButtonBox *>(); require(entry&&box,"Control operand picker controls missing");
 if (value.startsWith("Grid(")) {
  entry->setCurrentText("Grid"); dialog.findChild<QSpinBox *>("reportArrayRow")->setValue(1);
  dialog.findChild<QSpinBox *>("reportArrayColumn")->setValue(value.mid(value.indexOf(',')+1).chopped(1).toInt()); dialog.findChild<QPushButton *>("reportAddElement")->click();
 } else if (value.startsWith("Sat.")) {
  dialog.findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat"); dialog.findChild<QComboBox *>("reportPropertyType")->setCurrentText(value.section('.',-1));
  dialog.findChild<QComboBox *>("reportPropertyDependency")->setCurrentText("EarthMJ2000Eq"); dialog.findChild<QPushButton *>("reportUseReference")->click();
 } else entry->setEditText(value);
 events(); acceptanceView(dialog); require(dialog.selection()==QStringList{value}&&box->button(QDialogButtonBox::Ok)->isEnabled(),"Control operand picker selection differs from requested literal/reference"); box->button(QDialogButtonBox::Ok)->click();
}

int main(int argc,char **argv)
{
 QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaControlLogic");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, control-logic cases and case id");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath(),caseFile=QFileInfo(argv[2]).absoluteFilePath(); const QString id=argv[3];
  const auto cases=QJsonDocument::fromJson(read(caseFile)).object()["cases"].toArray(); QJsonObject spec;
  for (const auto &item:cases) if (item.toObject()["id"].toString()==id) { require(spec.isEmpty(),"Duplicate control logic case"); spec=item.toObject(); }
  require(!spec.isEmpty(),"Unknown control logic case"); const QString command=spec["command"].toString(),family=spec["family"].toString(); const bool range=family=="range",loop=command=="While";
  QDir::setCurrent(QFileInfo(startup).absolutePath()); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Control logic fixture unavailable");
  MainWindow window; require(window.initialize(startup),"Control logic runtime unavailable"); window.show(); events(); acceptanceView(window);
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *messages=window.findChild<QPlainTextEdit *>("messageWindow"); require(source&&area&&tree&&messages,"Control logic workspace missing");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  const auto report=files.filePath("logic results Δ.csv"),trials=files.filePath("range trials Δ.csv");
  const auto bounds=spec["bound_values"].toArray(); const double lo=range ? bounds[0].toDouble():1,step=range ? bounds[1].toDouble():1,hi=range ? bounds[2].toDouble():3;
  QString setup="% Control logic source sentinel Ω\nCreate Variable I J Input A B Left Right Count Gate Keep Lo Delta Hi;\nJ = -99;\nKeep = 41;\nLo = "+number(lo)+";\nDelta = "+number(step)+";\nHi = "+number(hi)+";\nCreate Array Grid[2,3];\nGrid(1,1) = "+number(lo)+";\nGrid(1,2) = "+number(step)+";\nGrid(1,3) = "+number(hi)+";\nCreate String Note;\nNote = 'retained';\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = "+number(range ? lo:7000)+";\nSat.Y = "+number(range ? hi:7000)+";\nSat.Z = 0;\nSat.VX = "+number(range ? step:0)+";\nSat.VY = 7;\nSat.VZ = 0;\nCreate ReportFile Values Trials;\nValues.Filename = '"+report+"';\nTrials.Filename = '"+trials+"';\n";
  for (const auto &name:QStringList{"Values","Trials"}) setup+=name+".WriteHeaders = false;\n"+name+".FixedWidth = false;\n"+name+".Delimiter = ',';\n"+name+".Precision = 17;\n";
  setup+="BeginMissionSequence;\nKeep = Keep + 1;\n";
  const QString oldCondition=loop&&family=="relation" ? "Gate == 1 & A > 5":"A > 5";
  QString before,after,mission;
  if (range) {
   before="For 'Decision' I = 7:1:8;"; const auto refs=spec["bounds"].toArray(); after="For 'Decision' J = "+refs[0].toString()+":"+refs[1].toString()+":"+refs[2].toString()+";";
   mission="Input = 0;\nCount = 0;\n"+before+" % preserve header\nCount = Count + J; % preserve body\nInput = Input + 1;\nReport Trials J Count Input;\nEndFor; % preserve end\nReport Values Input Count J Lo Delta Hi Grid(1,1) Grid(1,2) Grid(1,3) Keep;\n";
  } else {
   QString wanted; const auto conditions=spec["conditions"].toArray(); for (int i=0;i<conditions.size();++i) { const auto c=conditions[i].toObject(); if (i) wanted+=" "+c["join"].toString()+" "; wanted+=c["left"].toString()+" "+c["relation"].toString()+" "+c["right"].toString(); }
   before=command+" 'Decision' "+oldCondition+";"; after=command+" 'Decision' "+wanted+";";
   if (family=="relation") mission="For 'Input sweep' Input = -1:1;\nA = Input;\nB = 0;\nGrid(1,1) = Input;\nSat.X = Input;\nLeft = "+QString(spec["kind"].toString()=="Literal" ? "0":"Input")+";\nRight = "+QString(spec["kind"].toString()=="Literal" ? "Input":"0")+";\n";
   else mission="Input = 0;\nFor 'Left sweep' I = 0:1;\nFor 'Right sweep' J = 0:1;\nA = I;\nB = J;\nLeft = A;\nRight = B;\nGrid(1,1) = 41;\nSat.X = 7000;\n";
   mission+="Count = 0;\nGate = 1;\n"+before+" % preserve header\n";
   if (loop) mission+="Count = Count + 1; % preserve body\nGate = 0;\n"+QString(family=="logic" ? "A = 0;\nB = 0;\n":"")+"EndWhile; % preserve end\n";
   else mission+="Count = 1; % preserve true branch\nElse;\nCount = -1; % preserve false branch\nEndIf; % preserve end\n";
   mission+="Report Values Input Left Right Count Gate A B Grid(1,1) Sat.EarthMJ2000Eq.X Keep;\n";
   mission+=family=="logic" ? "Input = Input + 1;\nEndFor;\nEndFor;\n":"EndFor;\n";
  }
  const auto baseline=setup+mission+"% preserve tail Ω\n",expected=QString(baseline).replace(before,after);
  source->setPlainText(baseline); require(window.buildScript(),"Control logic baseline failed to build"); save("baseline.script",baseline.toUtf8()); save("case.json",QJsonDocument(spec).toJson());
  auto open=[&] {
   auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
   QTreeWidgetItem *item=nullptr; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->text(0).endsWith("Decision")) { require(!item,"Control logic tree label ambiguous"); item=*it; }
   require(item,"Control logic tree item missing"); tree->expandAll(); tree->setCurrentItem(item); tree->scrollToItem(item); events(); acceptanceView(window); tree->itemDoubleClicked(item,0); events();
   auto *child=area->activeSubWindow(); require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Actual control logic command editor missing"); child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); area->setActiveSubWindow(child); child->raise(); events(); acceptanceView(*child); return child;
  };
  auto *child=open(); auto panel=[&] { return dynamic_cast<CommandEditor *>(child->widget()); };
  if (range) {
   const auto original=child->findChild<QPlainTextEdit *>("commandSource")->toPlainText();
   for (bool accept:{false,true}) {
    bool visited=false; std::exception_ptr failure; QTimer::singleShot(0,child,[&] { auto *dialog=qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
     try { require(dialog&&dialog->comboBoxItems().contains("I")&&dialog->comboBoxItems().contains("J")&&!dialog->comboBoxItems().contains("Sat")&&!dialog->comboBoxItems().contains("Grid"),"For index selector lost Variable-only contract"); visited=true; dialog->setTextValue("J"); acceptanceView(*dialog); if (accept) dialog->accept(); else dialog->reject(); }
     catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
    }); child->findChild<QPushButton *>("commandChoose_Index")->click(); if (failure) std::rethrow_exception(failure); require(visited,"For index picker unvisited"); events(); acceptanceView(*child);
    if (!accept) require(child->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==original&&!panel()->hasChanges()&&source->toPlainText()==baseline,"For index Cancel changed draft/source");
   }
   const auto refs=spec["bounds"].toArray(); const QStringList fields{"Start","Step","End"};
   for (int i=0;i<3;++i) {
    bool visited=false; std::exception_ptr failure; QTimer::singleShot(0,child,[&] { auto *dialog=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());
     try { require(dialog,"For bound parameter picker missing"); visited=true; selectParameter(*dialog,refs[i].toString()); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
    }); auto *choose=child->findChild<QPushButton *>("commandChoose_"+fields[i]); require(choose&&choose->isVisible(),"For bound chooser not visible"); choose->click(); if (failure) std::rethrow_exception(failure); require(visited,"For bound picker unvisited"); events(); acceptanceView(*child);
   }
  } else {
   auto *field=child->findChild<QLineEdit *>("commandField_Condition"); auto *choose=child->findChild<QPushButton *>("commandChoose_Condition"); require(field&&field->isVisible()&&choose&&choose->isVisible()&&choose->isEnabled(),"Actual condition builder controls not visible");
   for (bool accept:{false,true}) {
    bool visited=false; std::exception_ptr failure; QTimer::singleShot(0,child,[&] { auto *dialog=dynamic_cast<ConditionDialog *>(QApplication::activeModalWidget());
     try { require(dialog,"Actual condition table dialog missing"); visited=true; auto *table=dialog->findChild<QTableWidget *>("conditionTable"); auto *box=dialog->findChild<QDialogButtonBox *>(); require(table&&box,"Condition table/buttons missing");
      if (!accept) { table->item(0,1)->setText("Discarded"); acceptanceView(*dialog); box->button(QDialogButtonBox::Cancel)->click(); return; }
      const auto conditions=spec["conditions"].toArray();
      while (table->rowCount()<conditions.size()) dialog->findChild<QPushButton *>("conditionAdd")->click();
      while (table->rowCount()>conditions.size()) { table->setCurrentCell(table->rowCount()-1,1); dialog->findChild<QPushButton *>("conditionRemove")->click(); }
      for (int row=0;row<conditions.size();++row) {
       const auto c=conditions[row].toObject(); auto *op=qobject_cast<QComboBox *>(table->cellWidget(row,2)),*join=qobject_cast<QComboBox *>(table->cellWidget(row,0));
       require(op&&join&&op->count()==6&&join->count()==2,"Condition table operator choices incomplete"); for (const auto &name:QStringList{"==","~=","<","<=",">",">="}) require(op->findText(name)>=0,"Condition table lacks required relation");
       table->item(row,1)->setText(c["left"].toString()); table->item(row,3)->setText(c["right"].toString()); op->setCurrentText(c["relation"].toString()); join->setCurrentIndex(c["join"].toString()=="|" ? 1:0);
      }
      // Exercise a real nested operand browser, including array/frame choices.
      const int selected=conditions.size()-1; table->setCurrentCell(selected,1); bool operandVisited=false; std::exception_ptr operandFailure;
      QTimer::singleShot(0,dialog,[&] { auto *picker=dynamic_cast<ReportParameterDialog *>(QApplication::activeModalWidget());
       try { require(picker,"Nested condition operand browser missing"); operandVisited=true; selectParameter(*picker,conditions[selected].toObject()["left"].toString()); }
       catch (...) { operandFailure=std::current_exception(); if (picker) picker->reject(); }
      }); dialog->findChild<QPushButton *>("conditionChoose")->click(); if (operandFailure) std::rethrow_exception(operandFailure); require(operandVisited,"Condition operand browser unvisited");
      dialog->findChild<QPushButton *>("conditionAdd")->click(); require(!box->button(QDialogButtonBox::Ok)->isEnabled(),"Empty condition row accepted");
      table->setCurrentCell(table->rowCount()-1,1); dialog->findChild<QPushButton *>("conditionRemove")->click(); events(); acceptanceView(*dialog);
      require(box->button(QDialogButtonBox::Ok)->isEnabled()&&command+" 'Decision' "+dialog->condition()+";"==after,"Condition table differs from independent header oracle"); box->button(QDialogButtonBox::Ok)->click();
     } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
    }); choose->click(); if (failure) std::rethrow_exception(failure); require(visited,"Condition table dialog unvisited"); events(); acceptanceView(*child);
    if (!accept) require(field->text()==oldCondition&&!panel()->hasChanges()&&source->toPlainText()==baseline,"Condition Cancel changed draft/source");
   }
  }
  require(panel()->hasChanges()&&source->toPlainText()==baseline&&child->findChild<QPlainTextEdit *>("commandSource")->toPlainText().contains(after),"Control logic edit escaped pending state or changed branch header");
  acceptanceView(*child); child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); events(); require(source->toPlainText()==expected&&!panel()->hasChanges(),"Control logic Apply changed unrelated source or lost accepted draft");
  const int applied=source->document()->availableUndoSteps(); child->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); events(); require(source->toPlainText()==expected&&source->document()->availableUndoSteps()==applied,"Control logic repeated Apply changed source/history"); require(child->close(),"Clean control logic panel did not close"); events();
  source->undo(); require(source->toPlainText()==baseline&&window.buildScript(),"Control logic Undo did not restore exact source/model"); source->redo(); require(source->toPlainText()==expected&&window.buildScript(),"Control logic Redo did not restore exact source/model");
  const auto saved=files.filePath(id+" Δ.script"),copy=files.filePath(id+" copy ü.script"); require(window.saveScriptTo(saved)&&read(saved)==expected.toUtf8()&&window.saveScriptTo(copy)&&read(copy)==expected.toUtf8()&&window.loadScript(copy)&&window.buildScript()&&source->toPlainText()==expected,"Control logic Unicode Save/Save As/reopen changed exact source"); save("accepted.script",expected.toUtf8());
  child=open();
  if (range) for (const auto &name:QStringList{"Index","Start","Step","End"}) { auto *field=child->findChild<QLineEdit *>("commandField_"+name); require(field&&field->isVisible(),"Reopened For typed field missing"); }
  else { auto *field=child->findChild<QLineEdit *>("commandField_Condition"); require(field&&field->isVisible()&&command+" 'Decision' "+field->text()+";"==after,"Reopened condition field lost exact header"); }
  require(child->close(),"Reopened clean control logic panel did not close"); events();
  const int undo=source->document()->availableUndoSteps(); QTimer watchdog; watchdog.setSingleShot(true); QObject::connect(&watchdog,&QTimer::timeout,&window,&MainWindow::stopMission); watchdog.start(10000);
  acceptanceView(window); const auto status=window.runMission(); watchdog.stop(); save("messages.txt",messages->toPlainText().toUtf8()); if (status!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString(); require(status==MainWindow::RunResult::Completed,"Control logic mission failed or watchdog stopped an unbounded loop");
  const auto bytes=read(report),trialBytes=QFileInfo::exists(trials) ? read(trials):QByteArray(); save("results.csv",bytes); save("trials.csv",trialBytes); std::cerr<<"CONTROL_LOGIC_RESULT "<<id.toStdString()<<' '<<bytes.constData()<<std::endl;
  const auto observed=rows(bytes,10),trialRows=rows(trialBytes,3); const auto oracle=spec["oracle_rows"].toArray(); require(observed.size()==oracle.size(),"Control logic report row count differs from independent oracle");
  for (int r=0;r<observed.size();++r) { const auto got=observed[r].toArray(),want=oracle[r].toArray(); require(got.size()==want.size(),"Control logic oracle width differs"); for (int c=0;c<got.size();++c) require(std::abs(got[c].toDouble()-want[c].toDouble())<1e-10,"Control logic value differs from independent truth/range oracle");
   if (!range) { const bool truth=family=="logic" ? spec["join"].toString()=="&" ? (got[1].toDouble()!=0&&got[2].toDouble()!=0):(got[1].toDouble()!=0||got[2].toDouble()!=0) : compare(got[1].toDouble(),spec["relation"].toString(),got[2].toDouble()); require(got[3].toDouble()==(loop ? int(truth):truth ? 1:-1),"Control logic branch/loop violates independent Boolean semantics"); }
   require(got[9].toDouble()==42,"Control logic literal sentinel changed");
  }
  if (range) { const auto wanted=spec["oracle_trials"].toArray(); require(trialRows.size()==wanted.size(),"For trial count differs from inclusive arithmetic progression"); for (int r=0;r<trialRows.size();++r) for (int c=0;c<3;++c) require(std::abs(trialRows[r].toArray()[c].toDouble()-wanted[r].toArray()[c].toDouble())<1e-10,"For index/sum/count differs from independent per-iteration oracle"); }
  require(source->toPlainText()==expected&&source->document()->availableUndoSteps()==undo,"Control logic Run changed exact source/history");
  const QJsonObject proof{{"checkpoint","control-logic"},{"outcome","passed"},{"case",id},{"command",command},{"family",family},{"kind",spec["kind"]},{"header",after},{"report_rows",observed},{"trials",trialRows},{"numeric_sentinel",42},
   {"facets",QJsonArray{"actual-mission-tree-command-editor","actual-condition-or-range-pickers","cancel-pending-apply-idempotence","exact-history-unicode-persistence-and-reopen","independent-truth-or-per-iteration-range-oracles"}},
   {"scope","Selected finite exact comparison boundaries and Boolean truth tables or inclusive fractional/negative/empty For ranges using literal/variable/array/same-frame numeric operands. For index remains Variable per Options table/runtime; conflicting general remarks are not qualified. No arbitrary expressions/index domains/precedence, numerical conditioning, propagation/frame science, native OS input, hardware/platform/stress/current private NASA corpus or full current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
