#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "BaseException.hpp"
#include "CommandEditor.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "QtPlotReceiver.hpp"
#include "FileManager.hpp"
#include "FiniteBurn.hpp"
#include <QCryptographicHash>
#include <QRegularExpression>
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QStatusBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextCursor>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
static void events() { QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path);require(f.open(QIODevice::ReadOnly),"Mission name artifact unavailable");return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Mission name artifact write failed"); }
struct Choice { QString title;QMessageBox::StandardButton answer; };
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaMissionCommandNames");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Supply startup, case JSON and command family");const QString family=argv[3];const auto caseBytes=read(QFileInfo(argv[2]).absoluteFilePath());const auto spec=QJsonDocument::fromJson(caseBytes).object();require(spec["family"]==family,"Unknown mission command naming case");const QString command=spec["command"].toString(),operand=spec["operand"].toString();const auto caseHash=QString::fromLatin1(QCryptographicHash::hash(caseBytes,QCryptographicHash::Sha256).toHex());
  TestSettings settings;QTemporaryDir files;require(files.isValid(),"Mission name fixture unavailable");const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),bytes);};
  constexpr double radius=7000,mu=398600.4415;const double speed=std::sqrt(mu/radius),omega=std::sqrt(mu/(radius*radius*radius));
  const auto pathA=files.filePath("Mission names Δ.script"),reportA=files.filePath("values.csv"),extra=files.filePath("extra.csv");
  QString initial=spec["script"].toString();for(const auto &pair:QList<QPair<QString,QString>>{{"@SPEED@",QString::number(speed,'g',17)},{"@VALUES@",reportA},{"@EXTRA@",extra},{"@DC@",files.filePath("dc.txt")},{"@OPT@",files.filePath("opt.txt")}})initial.replace(pair.first,pair.second);require(!initial.contains('@'),"Unresolved mission name fixture placeholder");write(pathA,initial.toUtf8());save("initial.script",initial.toUtf8());save("case.json",caseBytes);
  MainWindow window;require(window.initialize(startup),"Mission name runtime unavailable");window.show();events();acceptanceView(window);FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());require(std::abs(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant()-mu)<1e-9,"Mission name literal gravity differs");
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Mission name workspace absent");
  if(!window.loadScript(pathA)||!window.buildScript()){save("build-messages.txt",messages->toPlainText().toUtf8());throw std::runtime_error("Mission name initial Build failed");}
  auto configured=[&]{auto *sat=Moderator::Instance()->GetConfiguredObject("Sat"),*k=Moderator::Instance()->GetConfiguredObject("Keep");require(sat&&sat->IsOfType("Spacecraft")&&k&&k->GetRealParameter("Value")==41,"Mission name configured spacecraft/sentinel differ");};
  QJsonArray runs;auto run=[&]{configured();const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();const auto outcome=window.runMission();save("messages.txt",messages->toPlainText().toUtf8());require(outcome==MainWindow::RunResult::Completed,"Mission name renamed mission failed");const auto raw=read(reportA);save("values.csv",raw);const auto line=QString::fromUtf8(raw).trimmed();require(!line.contains('\n'),"Mission name report row count differs");const auto cells=line.split(',');require(cells.size()==12,"Mission name report width differs");QJsonArray values,expected;const double angle=omega*20;const auto impulse=spec["velocity_increment"].toArray();const double orbit[]={20,radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle)+impulse[0].toDouble(),speed*std::cos(angle)+impulse[1].toDouble(),impulse[2].toDouble(),41};for(double v:orbit)expected.append(v);for(auto v:spec["expected_tail"].toArray())expected.append(v);
   for(int i=0;i<12;++i){bool ok=false;const double value=cells[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<(i==0?1e-4:2e-5),"Mission name result differs from independent circle, impulse or algebraic/control oracle");values.append(value);}auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(sat,"Mission name native spacecraft missing");const char *axes[]={"X","Y","Z","VX","VY","VZ"};for(int i=0;i<6;++i)require(std::abs(sat->GetRealParameter(axes[i])-expected[i+1].toDouble())<2e-5,"Mission name native orbit differs");int index=7;for(const auto &name:QStringList{"Keep","Count","U","Cost","Quantity"}){auto *v=Moderator::Instance()->GetInternalObject(name.toStdString());require(v&&std::abs(v->GetRealParameter("Value")-expected[index++].toDouble())<2e-5,"Mission name native variable differs from independent control/solver oracle");}
   QJsonObject effects;const auto trace=window.plotReceiver()->model("Trace"),control=window.plotReceiver()->model("Control");require(trace&&control&&trace->curves.size()==1&&control->curves.size()==1,"Mission name publication model missing");for(const auto &m:{trace,control}){require(m->curves[0].points.size()>=2,"Mission name publication samples absent");for(const auto &p:m->curves[0].points)require(std::isfinite(p.x)&&std::abs(p.x-p.y)<1e-10,"Mission name elapsed publication differs");require(std::abs(m->curves[0].points.back().x-20)<1e-4,"Mission name final sample epoch differs");}
   if(family=="Toggle"){require(!trace->active&&control->active,"Mission name Toggle lost subscriber identity or Off state");effects.insert("toggle_active",trace->active);}
   if(family=="BeginFiniteBurn"||family=="EndFiniteBurn"){auto *burn=dynamic_cast<FiniteBurn *>(Moderator::Instance()->GetInternalObject("Burn"));require(burn&&!burn->IsFiring(),"Mission name finite-burn pair left burn firing");effects.insert("finite_burn_firing",burn->IsFiring());}
   if(family=="Report"){const auto bytes=read(extra);save("extra-values.txt",bytes);bool ok=false;const double value=QString::fromUtf8(bytes).trimmed().toDouble(&ok);require(ok&&value==41,"Mission name Report lost selected object/value");effects.insert("extra_report",value);}
   if(family=="Save"){const auto bytes=read(files.filePath("Sat.Spacecraft.data"));save("saved-runtime.txt",bytes);require(bytes.contains("Create Spacecraft Sat")&&bytes.contains("Sat.X")&&bytes.contains("Sat.VX"),"Mission name Save lost native spacecraft export");effects.insert("save_bytes",bytes.size());}
   require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Mission name execution changed source/history");runs.append(QJsonObject{{"values",values},{"expected",expected},{"effects",effects}});save("runs.json",QJsonDocument(runs).toJson());};
  QString currentLabel="Probe",statement=command+" 'Probe' "+operand,wanted=initial;require(wanted.count(statement)==1,"Mission name literal initial header ambiguous");int target=-1;for(int i=0;i<window.missionSnapshot().nodes.size();++i){const auto &n=window.missionSnapshot().nodes[i];if(n.type==command&&missionCommandLabel(n.statement).name=="Probe"){require(target<0,"Mission name native target ambiguous");target=i;}}require(target>=0,"Mission name target missing");const auto nativeInitial=window.missionSnapshot().nodes[target].statement;auto anonymous=[](QString text,const QString &name){if(!name.isEmpty())text.replace("'"+name+"'","");text.remove(QRegularExpression("\\s+"));return text;};const auto canonical=anonymous(nativeInitial,"Probe");
  auto item=[&]{for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);QTreeWidgetItem *found=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==target)found=*it;require(found,"Mission name tree target missing");tree->expandAll();tree->setCurrentItem(found);tree->scrollToItem(found);events();acceptanceView(*tree);return found;};
  auto identity=[&]{const auto &node=window.missionSnapshot().nodes[target];require(node.type==command&&missionCommandLabel(node.statement).name==currentLabel&&anonymous(node.statement,currentLabel)==canonical,"Mission name native command/branch operands changed");if(!currentLabel.isEmpty())require(item()->text(0)==command+" — "+currentLabel,"Mission name short tree label differs");else{const auto header=spec["anonymous_tree_label"].toString();require(!header.isEmpty(),"Mission name canonical anonymous label oracle missing");require(item()->text(0).simplified()==header.simplified(),"Mission name canonical anonymous tree identity differs");}configured();};
  auto context=[&](const QString &action){auto *entry=item();std::exception_ptr failure;bool visited=false;QTimer::singleShot(0,&window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{require(menu,"Mission name actual context menu absent");QAction *choice=nullptr;for(auto *a:menu->actions())if(a->objectName()==action||a->text()==action)choice=a;require(choice&&choice->isEnabled(),"Mission name context action absent or disabled");acceptanceView(*menu);menu->setActiveAction(choice);visited=true;QTest::keyClick(menu,Qt::Key_Return);}catch(...){failure=std::current_exception();if(menu)menu->close();}});tree->customContextMenuRequested(tree->visualItemRect(entry).center());events();if(failure)std::rethrow_exception(failure);require(visited,"Mission name context action not exercised");};
  auto open=[&]{context("Edit command…");auto *owner=area->currentSubWindow();auto *panel=owner?dynamic_cast<CommandEditor *>(owner->widget()):nullptr;auto *name=panel?panel->findChild<QLineEdit *>("commandName"):nullptr;require(panel&&name&&name->text()==currentLabel,"Mission name actual editor identity differs");acceptanceView(*owner);return QPointer<QMdiSubWindow>(owner);};
  auto closePanel=[&](QPointer<QMdiSubWindow> owner){auto *button=owner?owner->findChild<QPushButton *>("cancelPanel"):nullptr;require(button,"Mission name panel Cancel absent");QTest::mouseClick(button,Qt::LeftButton);events();require(!owner,"Mission name Cancel retained editor");};
  identity();auto owner=open();closePanel(owner);QJsonArray edits,rejections;const auto saved=files.filePath("Command names Ω.script");
  auto rename=[&](const QString &requested,const QString &outcome){const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();const bool modified=source->document()->isModified();std::exception_ptr failure;bool visited=false;QTimer timer;timer.setInterval(20);QObject::connect(&timer,&QTimer::timeout,&window,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());if(!dialog)return;timer.stop();try{require(dialog->objectName()=="renameCommandDialog","Mission name actual Rename dialog missing");auto *input=dialog->findChild<QLineEdit *>("commandRenameName");auto *status=dialog->findChild<QLabel *>("commandRenameStatus");auto *buttons=dialog->findChild<QDialogButtonBox *>();require(input&&input->text()==currentLabel&&status&&buttons,"Mission name Rename fields/default identity differ");acceptanceView(*dialog);input->setText(requested);visited=true;QTest::mouseClick(buttons->button(outcome=="cancel"?QDialogButtonBox::Cancel:QDialogButtonBox::Ok),Qt::LeftButton);if(outcome=="reject"){require(dialog->isVisible()&&input->text()==requested&&status->text().contains("Single quotes")&&source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Mission name invalid quote lost correction input, committed source or omitted diagnostic");rejections.append(QJsonObject{{"value",requested},{"diagnostic",status->text()}});QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);}}catch(...){failure=std::current_exception();dialog->reject();}});timer.start();context("missionRenameCommand");timer.stop();if(failure)std::rethrow_exception(failure);require(visited&&!QApplication::activeModalWidget(),"Mission name Rename dialog did not complete");
   if(outcome=="accept"&&requested.trimmed()!=currentLabel){auto next=statement;const auto label=requested.trimmed();if(currentLabel.isEmpty())next.insert(next.indexOf(operand),label.isEmpty()?QString():"'"+label+"' ");else next.replace("'"+currentLabel+"'",label.isEmpty()?QString():"'"+label+"'");require(wanted.count(statement)==1,"Mission name literal replacement ambiguous");wanted.replace(statement,next);statement=next;currentLabel=label;require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()>undo,"Mission name Rename changed unexpected source or omitted history");}
   else require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified,"Mission name Cancel/rejection/no-op changed source/history");identity();edits.append(QJsonObject{{"requested",requested},{"outcome",outcome},{"statement",statement},{"source",source->toPlainText()}});save("edits.json",QJsonDocument(edits).toJson());};
  rename("Cancelled pending name","cancel");rename("Invalid'Name","reject");rename("Probe","accept");
  const auto beforeRename=wanted;rename(QString::fromUtf8("Renamed Δ \"text\" : 42"),"accept");const auto renamed=wanted;source->undo();require(source->toPlainText()==beforeRename,"Mission name exact Undo failed");source->redo();require(source->toPlainText()==renamed&&window.buildScript(),"Mission name exact Redo/rebuild failed");identity();
  rename("Witness","accept");require(source->toPlainText().count("'Witness'")==2,"Mission name duplicate command label not accepted");identity();owner=open();auto *name=owner->findChild<QLineEdit *>("commandName");name->setText("Unapplied name");const auto pending=owner->findChild<QPlainTextEdit *>("commandSource")->toPlainText();const int guardUndo=source->document()->availableUndoSteps();context("missionRenameCommand");require(!QApplication::activeModalWidget()&&owner&&owner->findChild<QPlainTextEdit *>("commandSource")->toPlainText()==pending&&source->toPlainText()==wanted&&source->document()->availableUndoSteps()==guardUndo&&window.statusBar()->currentMessage().contains("Apply or discard"),"Mission name Rename bypassed pending-panel protection");closePanel(owner);
  rename("","accept");require(currentLabel.isEmpty()&&statement==command+"  "+operand,"Mission name empty label changed significant statement bytes");owner=open();closePanel(owner);
  rename(QString::fromUtf8("Final Ω name"),"accept");require(statement==command+QString::fromUtf8("  'Final Ω name' ")+operand,"Mission name insertion into anonymous command changed spacing");
  require(window.saveScriptTo(saved)&&read(saved)==wanted.toUtf8()&&window.loadScript(saved)&&source->toPlainText()==wanted&&window.buildScript(),"Mission name Unicode save/reopen changed source");identity();owner=open();closePanel(owner);run();require(source->toPlainText()==wanted&&!source->document()->isModified()&&read(saved)==wanted.toUtf8(),"Mission name final execution changed source/file");save("final.script",wanted.toUtf8());save("rejections.json",QJsonDocument(rejections).toJson());
  const QJsonObject proof{{"checkpoint","mission-command-names-"+family},{"outcome","passed"},{"family",family},{"command",command},{"case_sha256",caseHash},{"renames",edits},{"rejections",rejections},{"final_name",currentLabel},{"duplicate_name_allowed",true},{"pending_guard",true},{"raw_report_values",12},{"runs",runs},{"facets",QJsonArray{"actual-mission-tree-rename-dialog-and-command-editor-name","cancel-noop-exact-source-history-and-invalid-single-quote-correction","unicode-punctuation-empty-and-duplicate-command-labels","exact-undo-redo-unicode-save-reopen-native-tree-identities","pending-panel-rename-protection","independent-circle-impulse-sentinel-control-and-native-command-results"}},{"scope",spec["scope"]}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
