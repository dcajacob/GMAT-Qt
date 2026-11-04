#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "PathSettingsDialog.hpp"
#include "TestSettings.hpp"
#include "FileManager.hpp"
#include "GmatGlobal.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTabWidget>
#include <QTextDocument>
#include <QTimer>
#include <iostream>
#include <stdexcept>

static int assertions=0;
static void require(bool ok,const char *message){++assertions;if(!ok)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();}
static QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"MATLAB path artifact missing");return f.readAll();}
static void write(const QString &path,const QByteArray &data){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"MATLAB path artifact write failed");}
static QStringList paths(){QStringList values;for(const auto &s:FileManager::Instance()->GetAllMatlabFunctionPaths())values.append(QDir::cleanPath(QString::fromStdString(s)));return values;}
static QString lookup(const QString &name){const auto path=QString::fromStdString(FileManager::Instance()->GetMatlabFunctionPath(name.toStdString()));return path.isEmpty()?QString():QDir::cleanPath(path);}

int main(int argc,char **argv){
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaMatlabPaths");
 try{
  require(argc==2||argc==3||argc==4,"Supply startup or independent fresh-process lookup");const QString startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
  if(argc==4){require(QString::fromLocal8Bit(argv[2])=="--lookup","Unknown MATLAB path helper mode");FileManager::Instance()->ReadStartupFile(startup.toStdString());QJsonObject result{{"lookup",lookup(QString::fromLocal8Bit(argv[3]))},{"paths",QJsonArray::fromStringList(paths())}};std::cout<<"MATLAB_LOOKUP "<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;}
  const bool browseImport=argc==3;require(!browseImport||QString::fromLocal8Bit(argv[2])=="--browse-import","Unknown MATLAB path GUI mode");
  require(acceptanceWidgetPlatform(),"MATLAB path widget platform missing");TestSettings settings;QTemporaryDir files;require(files.isValid(),"MATLAB path private directory missing");
  const auto first=files.filePath("first MATLAB α"),second=files.filePath("second MATLAB β"),empty=files.filePath("empty MATLAB γ"),exported=files.filePath("exported startup Δ.txt");
  for(const auto &p:QStringList{first,second,empty})require(QDir().mkpath(p),"MATLAB path directory creation failed");
  const QByteArray one="function y = AcceptancePriority(x)\ny = x * 2;\nend\n",two="function y = AcceptancePriority(x)\ny = x * 3;\nend\n";
  write(first+"/AcceptancePriority.m",one);write(second+"/AcceptancePriority.m",two);
  MainWindow window;require(window.initialize(startup),"MATLAB path runtime initialization failed");window.show();acceptanceView(window);events();
  auto *manager=FileManager::Instance();const auto state=manager->CaptureState();struct Restore{std::shared_ptr<const FileManager::State> state;~Restore(){FileManager::Instance()->RestoreState(*state);}}restore{state};
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");auto *action=window.findChild<QAction *>("setPaths");require(source&&action&&action->isEnabled(),"MATLAB path workspace missing");
  const QString baseline="% Preserve unrelated source Ω\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nKeep = Keep + 1;\n";source->setPlainText(baseline);require(window.buildScript(),"MATLAB path baseline failed");const auto undo=source->document()->availableUndoSteps();const auto original=paths();
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),data);};save("first.m",one);save("second.m",two);save("baseline.script",baseline.toUtf8());QJsonArray observations;
  auto inspect=[&](auto body){std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,&window,[&]{auto *owner=dynamic_cast<PathSettingsDialog *>(QApplication::activeModalWidget());try{
   require(owner,"Actual MATLAB Set paths dialog missing");auto *tabs=owner->findChild<QTabWidget *>("pathTabs");auto *tab=owner->findChild<QWidget *>("pathMatlabTab");auto *list=owner->findChild<QListWidget *>("pathMatlabFunctions");auto *entry=owner->findChild<QLineEdit *>("pathMatlabEntry");require(tabs&&tab&&list&&entry,"Actual MATLAB function path controls missing");tabs->setCurrentWidget(tab);events();acceptanceView(*owner);require(list->isVisible()&&entry->isVisible(),"MATLAB function path controls not visible");visited=true;body(owner,list,entry);
  }catch(...){failure=std::current_exception();}if(owner)owner->reject();});action->trigger();events();if(failure)std::rethrow_exception(failure);require(visited,"MATLAB path dialog unvisited");require(source->toPlainText()==baseline&&source->document()->availableUndoSteps()==undo,"MATLAB path operation changed mission source/history");};
  auto click=[&](QWidget *owner,const char *name){auto *button=owner->findChild<QPushButton *>(name);require(button&&button->isEnabled()&&button->isVisible(),"MATLAB path action missing");button->click();events();};
  auto clear=[&](QWidget *owner,QListWidget *list){while(list->count()){list->setCurrentRow(0);click(owner,"pathMatlabRemove");}};
  auto add=[&](QWidget *owner,QLineEdit *entry,const QString &path){entry->setText(path);click(owner,"pathMatlabAdd");};
  auto apply=[&](QWidget *owner){auto *buttons=owner->findChild<QDialogButtonBox *>();require(buttons,"MATLAB path buttons missing");buttons->button(QDialogButtonBox::Apply)->click();events();require(owner->isVisible(),"MATLAB path Apply closed the owner");};
  if(browseImport){
   auto choose=[&](QWidget *owner,const char *button,const QString &path,bool cancel){std::exception_ptr failure;bool visited=false;const auto before=owner->findChild<QLineEdit *>("pathMatlabEntry")->text();QTimer::singleShot(0,owner,[&]{auto *dialog=dynamic_cast<QFileDialog *>(QApplication::activeModalWidget());try{require(dialog,"Actual MATLAB path file/directory chooser missing");visited=true;acceptanceView(*dialog);if(cancel)dialog->reject();else{dialog->setDirectory(QFileInfo(path).absolutePath());auto *filename=dialog->findChild<QLineEdit *>("fileNameEdit");require(filename,"MATLAB chooser filename field missing");filename->setText(path);QTimer guard;guard.setInterval(10);QObject::connect(&guard,&QTimer::timeout,dialog,[&]{if(auto *message=dynamic_cast<QMessageBox *>(QApplication::activeModalWidget())){failure=std::make_exception_ptr(std::runtime_error(("MATLAB chooser rejected prescribed path: "+message->text()).toStdString()));message->accept();dialog->reject();}});guard.start();require(QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection),"MATLAB chooser accept delivery failed");}}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});click(owner,button);if(failure)std::rethrow_exception(failure);require(visited,"MATLAB chooser unvisited");if(cancel)require(owner->findChild<QLineEdit *>("pathMatlabEntry")->text()==before,"MATLAB Browse Cancel changed pending text");};
   inspect([&](PathSettingsDialog *owner,QListWidget *list,QLineEdit *entry){clear(owner,list);entry->setText("retained pending text");choose(owner,"pathMatlabBrowse",first,true);choose(owner,"pathMatlabBrowse",first,false);require(QDir::cleanPath(entry->text())==first,"MATLAB directory Browse chose wrong path");click(owner,"pathMatlabAdd");choose(owner,"pathMatlabBrowse",second,false);click(owner,"pathMatlabAdd");require(paths()==original,"Pending MATLAB Browse changed native list");apply(owner);require(paths()==QStringList{second,first}&&lookup("AcceptancePriority")==second,"MATLAB Browse Apply/native lookup failed");require(owner->exportStartup(exported).isEmpty(),"MATLAB Browse startup export failed");save("exported-startup.txt",read(exported));});
   inspect([&](PathSettingsDialog *owner,QListWidget *list,QLineEdit *){clear(owner,list);apply(owner);require(paths().isEmpty(),"MATLAB import fixture did not clear native list");choose(owner,"pathReadStartup",exported,false);require(paths().isEmpty(),"Reading pending MATLAB startup changed native paths");require(list->count()==2&&list->item(0)->text()==second&&list->item(1)->text()==first,"Imported MATLAB startup order differs");apply(owner);require(paths()==QStringList{second,first}&&lookup("AcceptancePriority")==second&&read(second+"/AcceptancePriority.m")==two,"Imported MATLAB path/native prescribed file differs");});
   QJsonObject proof{{"checkpoint","matlab-function-paths"},{"mode","BrowseImport"},{"outcome","passed"},{"assertions",assertions},{"native_paths",QJsonArray::fromStringList(paths())},{"native_lookup",lookup("AcceptancePriority")},{"matlab_function_execution_claimed",false},{"facets",QJsonArray{"actual-native-directory-and-file-choosers","Browse-Cancel-pending-isolation","startup-import-transaction-and-order","native-prescribed-file-lookup","unrelated-source-history-isolation"}}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';return 0;
  }
  inspect([&](PathSettingsDialog *owner,QListWidget *list,QLineEdit *entry){clear(owner,list);add(owner,entry,first);require(list->count()==1&&paths()==original,"Pending MATLAB list changed the native paths");});require(paths()==original,"MATLAB path Cancel changed native paths");
  inspect([&](PathSettingsDialog *owner,QListWidget *list,QLineEdit *entry){clear(owner,list);add(owner,entry,first);add(owner,entry,second);require(list->count()==2&&list->item(0)->text()==second,"MATLAB Add priority wrong");apply(owner);require(paths()==QStringList{second,first},"MATLAB Apply order differs from displayed order");require(lookup("AcceptancePriority")==second&&read(second+"/AcceptancePriority.m")==two,"Actual native MATLAB lookup chose wrong file");observations.append(QJsonObject{{"phase","second-first"},{"lookup",lookup("AcceptancePriority")}});
   add(owner,entry,second+"/.");require(list->count()==2&&!owner->findChild<QLabel *>("pathStatus")->text().isEmpty(),"MATLAB duplicate directory accepted");add(owner,entry,files.filePath("missing"));require(list->count()==2,"MATLAB missing directory accepted");
   list->setCurrentRow(1);click(owner,"pathMatlabUp");require(paths()==QStringList{second,first},"Pending MATLAB reorder changed native paths");apply(owner);require(paths()==QStringList{first,second}&&lookup("AcceptancePriority.m")==first,"MATLAB reorder did not change native priority");
   list->setCurrentRow(1);entry->setText(empty);click(owner,"pathMatlabReplace");list->setCurrentRow(1);click(owner,"pathMatlabUp");apply(owner);require(paths()==QStringList{empty,first}&&lookup("AcceptancePriority")==first,"MATLAB empty-directory fallback failed");
   list->setCurrentRow(0);list->currentItem()->setText(files.filePath("missing invalid Apply"));apply(owner);require(paths()==QStringList{empty,first}&&!owner->findChild<QLabel *>("pathStatus")->text().isEmpty(),"Invalid MATLAB Apply changed native paths");list->currentItem()->setText(empty);apply(owner);require(paths()==QStringList{empty,first},"MATLAB Apply did not recover after invalid input");
   const auto error=owner->exportStartup(exported);require(error.isEmpty(),"MATLAB startup export failed");save("exported-startup.txt",read(exported));
  });
  require(lookup("MissingAcceptanceFunction").isEmpty(),"Missing MATLAB function unexpectedly resolved");
  inspect([&](PathSettingsDialog *owner,QListWidget *list,QLineEdit *){require(list->count()==2&&list->item(0)->text()==empty&&list->item(1)->text()==first,"Reopened MATLAB paths differ");list->setCurrentRow(1);click(owner,"pathMatlabRemove");apply(owner);require(paths()==QStringList{empty}&&lookup("AcceptancePriority").isEmpty(),"MATLAB Remove did not remove native lookup");clear(owner,list);apply(owner);require(paths().isEmpty(),"Empty MATLAB path list could not be applied");});
  QProcess child;child.setProcessChannelMode(QProcess::MergedChannels);child.start(QCoreApplication::applicationFilePath(),{exported,"--lookup","AcceptancePriority"});require(child.waitForStarted(5000)&&child.waitForFinished(30000)&&child.exitStatus()==QProcess::NormalExit&&child.exitCode()==0,"Fresh-process MATLAB lookup failed");const auto childText=child.readAll();save("fresh-process.log",childText);QJsonObject fresh;
  for(const auto &line:childText.split('\n'))if(line.startsWith("MATLAB_LOOKUP ")){require(fresh.isEmpty(),"Duplicate fresh MATLAB lookup");fresh=QJsonDocument::fromJson(line.mid(14)).object();}
  require(fresh["lookup"].toString()==first&&fresh["paths"].toArray()==QJsonArray{empty,first},"Exported MATLAB paths/order did not survive fresh startup");
  require(window.runMission()==MainWindow::RunResult::Completed,"Unrelated mission failed after MATLAB path operations");auto *keep=Moderator::Instance()->GetInternalObject("Keep");require(keep&&keep->GetRealParameter("Value")==42&&source->toPlainText()==baseline&&source->document()->availableUndoSteps()==undo,"MATLAB paths changed unrelated native mission/source/history");
  QJsonObject proof{{"checkpoint","matlab-function-paths"},{"mode","Operations"},{"outcome","passed"},{"assertions",assertions},{"observations",observations},{"fresh",fresh},{"matlab_function_execution_claimed",false},{"facets",QJsonArray{"actual-path-list-controls","pending-cancel-atomic-invalid-recovery","add-replace-remove-reorder-duplicate-validation","native-prescribed-file-priority-and-fallback","Unicode-export-fresh-process-lookup","unrelated-mission-source-history-isolation"}}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';
 }catch(BaseException &e){std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n';return 1;}catch(std::exception &e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}return 0;
}
