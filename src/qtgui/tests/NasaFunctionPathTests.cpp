#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Function path artifact unavailable"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Function path artifact write failed"); }
static QStringList paths() { QStringList result; for (const auto &p:FileManager::Instance()->GetAllGmatFunctionPaths()) result.append(QDir::cleanPath(QString::fromStdString(p))); return result; }

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("NasaFunctionPaths");
 try {
  require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and function path case");
  const QString mode=argv[2],startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  require(QStringList{"List","Search","Duplicate","Insert"}.contains(mode),"Unknown function path case");
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Function path fixture directory unavailable");
  const auto first=files.filePath("first functions α"),second=files.filePath("second functions β"),empty=files.filePath("empty directory"),missions=files.filePath("missions Δ");
  for (const auto &p:QStringList{first,second,empty,missions}) require(QDir().mkpath(p),"Function path directory creation failed");
  const QByteArray twice="function [y] = Priority(x)\nCreate Variable y;\nBeginMissionSequence;\ny = x * 2;\n";
  const QByteArray thrice="function [y] = Priority(x)\nCreate Variable y;\nBeginMissionSequence;\ny = x * 3;\n";
  write(first+"/Priority.gmf",twice); write(second+"/Priority.gmf",thrice);
  MainWindow window; require(window.initialize(startup),"Function path runtime unavailable"); window.show(); acceptanceView(window); events();
  auto *manager=FileManager::Instance(); const auto savedState=manager->CaptureState();
  struct Restore { std::shared_ptr<const FileManager::State> state; ~Restore() { FileManager::Instance()->RestoreState(*state); } } restore{savedState};
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace"); auto *tree=window.findChild<QTreeWidget *>("Mission"); auto *action=window.findChild<QAction *>("setPaths");
  require(source&&area&&tree&&action&&action->isEnabled(),"Function path workspace controls missing");
  const QString baseline="Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
  source->setPlainText(baseline); require(window.buildScript(),"Function path baseline failed"); const int baselineUndo=source->document()->availableUndoSteps();
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  save("first.gmf",twice); save("second.gmf",thrice); QJsonArray observations,reports;
  auto dialog=[&](std::function<void(QDialog *,QListWidget *,QLineEdit *)> inspect) {
   acceptanceView(window); std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&window,[&] {
    auto *owner=qobject_cast<QDialog *>(QApplication::activeModalWidget());
    try {
     require(owner&&owner->objectName()=="pathSettingsDialog","Actual Set paths dialog missing");
     auto *tabs=owner->findChild<QTabWidget *>("pathTabs"); auto *list=owner->findChild<QListWidget *>("pathFunctions"); auto *entry=owner->findChild<QLineEdit *>("pathFunctionEntry");
     require(tabs&&list&&entry,"Function path controls missing"); tabs->setCurrentIndex(0); events(); acceptanceView(*owner);
     require(list->isVisible()&&entry->isVisible(),"Function path list or entry not visible"); visited=true; inspect(owner,list,entry);
    } catch (...) { failure=std::current_exception(); }
    if (owner) owner->reject();
   });
   action->trigger(); events(); if (failure) std::rethrow_exception(failure); require(visited,"Set paths dialog was not exercised");
  };
  auto button=[&](QWidget *owner,const char *name) { auto *b=owner->findChild<QPushButton *>(name); require(b&&b->isVisible()&&b->isEnabled(),"Function path operation unavailable"); b->click(); events(); };
  auto clear=[&](QDialog *owner,QListWidget *list) { while (list->count()) { list->setCurrentRow(0); button(owner,"pathFunctionRemove"); } };
  auto add=[&](QDialog *owner,QLineEdit *entry,const QString &p) { entry->setText(p); button(owner,"pathFunctionAdd"); acceptanceView(*owner); };
  auto apply=[&](QDialog *owner) { auto *box=owner->findChild<QDialogButtonBox *>(); require(box,"Path dialog buttons missing"); acceptanceView(*owner); box->button(QDialogButtonBox::Apply)->click(); events(); require(owner->isVisible(),"Path Apply unexpectedly closed its owner"); };
  const auto originalPaths=paths();
  if (mode=="List") {
   dialog([&](QDialog *owner,QListWidget *list,QLineEdit *entry) { clear(owner,list); add(owner,entry,first); add(owner,entry,second); require(list->count()==2&&list->item(0)->text()==second,"Pending path list order differs from Add semantics"); });
   require(paths()==originalPaths,"Closing uncommitted path list changed the session");
  }
  const QStringList selected=mode=="Search" ? QStringList{empty,first} : QStringList{first,second};
  dialog([&](QDialog *owner,QListWidget *list,QLineEdit *entry) {
   clear(owner,list);
   for (int i=selected.size()-1;i>=0;--i) add(owner,entry,selected[i]);
   entry->setText(first+"/"); button(owner,"pathFunctionAdd"); require(list->count()==2,"Equivalent duplicate path was added");
   entry->setText(files.filePath("nonexistent")); button(owner,"pathFunctionAdd"); require(list->count()==2,"Unavailable function directory was added");
   apply(owner); require(paths()==selected,"Applied function path order differs from visible list");
  });
  require(source->toPlainText()==baseline&&source->document()->availableUndoSteps()==baselineUndo,"Function path controls changed source/history");
  observations.append(QJsonObject{{"operation","configure"},{"directory_order",QJsonArray::fromStringList(selected)},{"duplicate_rejected",true},{"missing_rejected",true},{"cancel_isolated",mode=="List"}});
  const auto report=files.filePath("function path values.csv");
  const QString explicitPath="Priority.FunctionPath = '"+first+"/Priority.gmf';\n";
  QString mission="% Function path acceptance Ω\nCreate GmatFunction Priority;\n"+((mode=="List"||mode=="Insert") ? explicitPath:QString())+
   "Create Variable x y Keep;\nx = 5;\ny = -1;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nBeginMissionSequence;\nKeep = Keep + 1;\n"+
   (mode=="Insert" ? QString():QString("[y] = Priority(x);\n"))+"Report 'Witness' Values x y Keep;\n% retained tail Ω\n";
  source->setPlainText(mission); require(window.buildScript(),"Function path mission failed to build");
  if (mode=="Insert") {
   auto *tabs=window.findChild<QTabWidget *>(); for (int i=0;tabs&&i<tabs->count();++i) if (tabs->tabText(i)=="Mission") tabs->setCurrentIndex(i);
   QTreeWidgetItem *anchor=nullptr; for (QTreeWidgetItemIterator it(tree);*it;++it) if ((*it)->text(0).contains("Witness")) anchor=*it;
   require(anchor,"Function insertion report anchor missing"); tree->expandAll(); tree->setCurrentItem(anchor); tree->scrollToItem(anchor); events(); acceptanceView(window);
   std::exception_ptr failure; bool visited=false;
   QTimer::singleShot(0,&window,[&] { try {
    auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget()); require(menu,"Function insertion popup missing"); QAction *choice=nullptr;
    for (auto *a:menu->actions()) if (a->text()=="Insert before…") choice=a;
    require(choice&&choice->isEnabled(),"Function insertion menu route unavailable"); menu->setActiveAction(choice); acceptanceView(*menu); visited=true;
    QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(menu,&enter);
   } catch (...) { failure=std::current_exception(); if (auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget())) menu->close(); } });
   tree->customContextMenuRequested(tree->visualItemRect(anchor).center()); events(); if (failure) std::rethrow_exception(failure); require(visited,"Function insertion popup not exercised");
   auto *child=area->activeSubWindow(); require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Function insertion actual command editor missing");
   child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); area->setActiveSubWindow(child); child->raise(); events(); acceptanceView(*child);
   auto *templates=child->findChild<QComboBox *>("commandTemplate"); require(templates&&templates->findText("Call function")>=0,"GMAT function insertion template missing");
   templates->showPopup(); auto *choices=templates->view(); choices->setCurrentIndex(templates->model()->index(templates->findText("Call function"),0)); acceptanceView(*choices);
   QKeyEvent choose(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier); QApplication::sendEvent(choices,&choose); events(); acceptanceView(*child);
   auto *preview=child->findChild<QPlainTextEdit *>("commandSource"); require(preview&&preview->toPlainText()=="[OutputVariable] = Priority(InputVariable);","Function template did not select configured GmatFunction");
   auto *inputs=child->findChild<QLineEdit *>("commandField_Inputs"),*outputs=child->findChild<QLineEdit *>("commandField_Outputs"); require(inputs&&outputs&&inputs->isVisible()&&outputs->isVisible(),"Function call form argument controls not visible");
   inputs->setText("x"); outputs->setText("[y]"); events(); acceptanceView(*child); require(preview->toPlainText()=="[y] = Priority(x);"&&source->toPlainText()==mission,"Pending function form changed source or call syntax");
   auto *box=child->findChild<QDialogButtonBox *>(); require(box,"Function command Apply buttons missing"); box->button(QDialogButtonBox::Apply)->click(); events();
   const auto expected=QString(mission).replace("Report 'Witness'","[y] = Priority(x);\nReport 'Witness'");
   require(source->toPlainText()==expected&&!dynamic_cast<CommandEditor *>(child->widget())->hasChanges(),"Function insertion Apply differs from exact source oracle");
   const int steps=source->document()->availableUndoSteps(); box=child->findChild<QDialogButtonBox *>(); require(box,"Applied function editor buttons missing"); box->button(QDialogButtonBox::Apply)->click(); require(source->toPlainText()==expected&&source->document()->availableUndoSteps()==steps,"Second function insertion Apply duplicated command/history");
   require(child->close(),"Applied function command did not close"); events(); source->undo(); require(source->toPlainText()==mission&&window.buildScript(),"Function insertion Undo lost exact source/model"); source->redo(); require(source->toPlainText()==expected&&window.buildScript(),"Function insertion Redo lost exact source/model"); mission=expected;
   observations.append(QJsonObject{{"operation","insert"},{"template","Call function"},{"call","[y] = Priority(x);"},{"undo_redo_exact",true},{"idempotent_apply",true}});
  }
  auto run=[&](const QString &name,int expected) {
   const int undo=source->document()->availableUndoSteps(); const auto accepted=source->toPlainText(); acceptanceView(window);
   require(window.runMission()==MainWindow::RunResult::Completed,"Function path mission execution failed");
   const QByteArray literal=QByteArray("5,")+QByteArray::number(expected)+",42"; require(read(report).trimmed()==literal,"Function path result differs from independent literal product and sentinel");
   require(source->toPlainText()==accepted&&source->document()->availableUndoSteps()==undo,"Function execution changed source/history"); save(name+".csv",read(report)); save(name+".script",accepted.toUtf8()); reports.append(QJsonObject{{"phase",name},{"values",QJsonArray{5,expected,42}}});
  };
  const auto saved=missions+"/function paths ü.script"; require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==mission,"Function path Unicode save/reopen changed source");
  run("first",10);
  if (mode=="Duplicate") {
   const int undo=source->document()->availableUndoSteps();
   dialog([&](QDialog *owner,QListWidget *list,QLineEdit *) {
    const QStringList expected{missions,QFileInfo(startup).absolutePath(),first,second}; QStringList actual; for (int i=0;i<list->count();++i) actual.append(QDir::cleanPath(list->item(i)->text()));
    require(actual==expected,"Duplicate lookup list has wrong initial order"); list->setCurrentRow(2); button(owner,"pathFunctionDown"); acceptanceView(*owner); apply(owner);
    require(paths()==QStringList{missions,QFileInfo(startup).absolutePath(),second,first},"Path Down did not change applied search order");
   });
   require(source->toPlainText()==mission&&source->document()->availableUndoSteps()==undo,"Path reorder changed mission source/history"); run("reordered",15);
   source->setPlainText(QString(mission).replace("Create GmatFunction Priority;\n","Create GmatFunction Priority;\n"+explicitPath)); require(window.buildScript(),"Explicit override mission failed to build"); run("explicit",10);
   observations.append(QJsonObject{{"operation","reorder-and-override"},{"first",10},{"reordered",15},{"explicit",10}});
  }
  require(read(first+"/Priority.gmf")==twice&&read(second+"/Priority.gmf")==thrice,"Function lookup or execution modified a function file");
  const QJsonObject proof{{"checkpoint","function-path-"+mode},{"mode",mode},{"outcome","passed"},{"numeric_sentinel",42},{"reports",reports},{"observations",observations},{"facets",QJsonArray{"actual-path-dialog-controls","source-history-isolation","independent-literal-products","unicode-persistence","native-widget-boundaries"}},{"scope","Selected editable path-list, missing-first-directory fallback, duplicate-name order/explicit override, or actual function command insertion. No arbitrary filesystem permissions/case/links/races, full MATLAB/function domain, native OS input, platform/stress/current private NASA corpus or full current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
