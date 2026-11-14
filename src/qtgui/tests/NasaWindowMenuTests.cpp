#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "EditablePanel.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTemporaryDir>
#include <QSet>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Window fixture/result missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Window artifact write failed"); }
static QJsonArray rect(const QRect &r) { return {r.x(),r.y(),r.width(),r.height()}; }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaWindowMenu");
 try {
  require(argc==3&&acceptanceWidgetPlatform(),"Expected startup and window-menu case");
  const QString mode=argv[2],startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Window fixture directory unavailable");
  const auto output=files.filePath("Window mission Δ.txt");
  const QString mission="Create Variable Keep;\nKeep = 9;\nCreate ReportFile Values;\nValues.Filename = '"+output+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nReport 'Literal witness' Values Keep; % preserve mission Ω\n";
  MainWindow window; require(window.initialize(startup),"Window runtime initialization failed"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace");
  require(source&&area,"Window workspace missing"); source->setPlainText(mission); require(window.buildScript(),"Window mission build failed");
  require(window.saveScriptTo(files.filePath("Active Δ.script")),"Active window save failed");
  const int originalUndo=source->document()->availableUndoSteps();
  QJsonArray observations,actions; const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  save("initial.script",mission.toUtf8());
  auto invariant=[&] {
   auto *keep=Moderator::Instance()->GetConfiguredObject("Keep");
   require(keep&&keep->GetRealParameter("Value")==9&&source->toPlainText()==mission&&source->document()->availableUndoSteps()==originalUndo&&!source->document()->isModified(),"Window menu changed active mission, configured literal or history");
  };
  struct Document { QString path,text; QPointer<QMdiSubWindow> child; QPointer<QPlainTextEdit> editor; };
  QList<Document> docs;
  for (int i=0;i<2;++i) {
   const auto path=files.filePath(QString("Inactive %1 Δ.script").arg(i)),text=QString("% Independent window buffer %1 Ω\n").arg(i);
   write(path,text.toUtf8()); require(window.openScriptDocument(path),"Independent script window open failed");
   auto *child=area->currentSubWindow(); auto *editor=qobject_cast<QPlainTextEdit *>(child->widget());
   require(editor&&editor!=source&&editor->toPlainText()==text,"Independent buffer unavailable"); docs.append({path,text,child,editor});
  }
  auto *menu=window.findChild<QMenu *>("windowMenu"); require(menu,"Window menu missing");
  auto invoke=[&](const QString &name) {
   acceptanceView(window); QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(menu->menuAction()).center()); events();
   require(menu->isVisible(),"Actual Window menu did not open"); auto *action=menu->findChild<QAction *>(name);
   require(action&&action->isEnabled(),"Window menu action unavailable"); acceptanceView(*menu);
   QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center()); events(); actions.append(name);
  };
  auto choose=[&](const QString &name,QMessageBox::StandardButton choice) {
   std::exception_ptr failure; int count=0; QTimer timer; timer.setInterval(10);
   QObject::connect(&timer,&QTimer::timeout,&window,[&] {
    auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (!box) return;
    try { acceptanceView(*box); auto *button=box->button(choice); require(button,"Requested close decision unavailable"); ++count; QTest::mouseClick(button,Qt::LeftButton); }
    catch (...) { failure=std::current_exception(); box->reject(); }
   });
   timer.start(); invoke(name); timer.stop(); if (failure) std::rethrow_exception(failure); require(count>0,"Close operation omitted its pending/dirty decision");
   observations.append(QJsonObject{{"decision",int(choice)},{"dialogs",count}});
  };
  auto visible=[&] { QList<QMdiSubWindow *> result; for (auto *child:area->subWindowList()) if (child->isVisible()&&!child->isMinimized()) result.append(child); return result; };
  auto states=[&] { QJsonArray result; for (auto *child:visible()) result.append(QJsonObject{{"title",child->windowTitle()},{"rect",rect(child->geometry())}}); return result; };
  auto checkBuffers=[&] { for (const auto &d:docs) if (d.editor) require(d.editor->toPlainText()==d.text&&!d.editor->document()->isModified()&&read(d.path)==d.text.toUtf8(),"Window layout/navigation altered an independent buffer or disk file"); };
  auto activate=[&](QMdiSubWindow *child) { require(child,"Window activation target missing"); child->showNormal(); area->setActiveSubWindow(child); child->widget()->setFocus(); events(); };
  if (mode=="TileVertical"||mode=="TileHorizontal") {
   const bool vertical=mode=="TileVertical"; auto children=visible(); require(children.size()==3,"Tile fixture has unexpected windows");
   // Include an initially maximized window and a second arrangement after resize.
   docs.first().child->showMaximized(); events();
   for (int pass=0;pass<2;++pass) {
    if (pass) { window.resize(window.width()-90,window.height()-60); events(); }
    invoke(vertical ? "tileWindowsVertically" : "tileWindowsHorizontally"); acceptanceView(window);
    children=visible(); const auto bounds=area->viewport()->rect(); int sum=0; QList<QRect> rectangles;
    for (auto *child:children) {
     const auto r=child->geometry(); require(!child->isMaximized()&&bounds.contains(r),"Tiled window is maximized or outside workspace");
     require(vertical ? (r.y()==0&&r.height()==bounds.height()) : (r.x()==0&&r.width()==bounds.width()),"Tiling orientation does not match menu choice");
     for (const auto &previous:rectangles) require(!previous.intersects(r),"Tiled windows overlap");
     rectangles.append(r); sum+=r.width()*r.height();
    }
    require(sum==bounds.width()*bounds.height(),"Tile layout does not exactly cover workspace");
    observations.append(QJsonObject{{"viewport",rect(bounds)},{"windows",states()}}); invariant(); checkBuffers();
   }
  } else if (mode=="Cascade") {
   invoke("cascadeWindows"); acceptanceView(window); QSet<QPoint> origins;
   for (auto *child:visible()) { require(!child->isMaximized()&&area->viewport()->rect().contains(child->geometry()),"Cascade leaves an unreachable window"); origins.insert(child->pos()); }
   require(origins.size()==3,"Cascade did not stagger every window"); observations.append(states()); invariant(); checkBuffers();
  } else if (mode=="Next"||mode=="Previous") {
   const auto order=area->subWindowList(area->activationOrder()); require(order.size()==3,"Window navigation fixture incomplete"); activate(order.first());
   const bool next=mode=="Next"; const auto action=next ? "nextWindow" : "previousWindow";
   for (int i=1;i<=3;++i) { invoke(action); require(area->currentSubWindow()==order[next ? i%3 : (3-i)%3],"Window navigation order or wraparound wrong"); observations.append(area->currentSubWindow()->windowTitle()); invariant(); checkBuffers(); }
  } else if (mode=="CloseCurrent") {
   activate(docs.first().child); invoke("closeCurrentWindow"); require(!docs.first().child&&docs.last().child&&docs.last().child->isVisible(),"Close current removed another window or retained its target"); invariant(); checkBuffers();
  } else if (mode=="CloseAll") {
   invoke("closeAllWindows"); require(visible().isEmpty()&&!docs.first().child&&!docs.last().child,"Close all retained a visible/deletable document"); invariant();
  } else if (mode=="DirtyCloseCurrent") {
   auto &d=docs.first(); const auto original=d.text; d.editor->appendPlainText("% pending edit Δ"); const auto dirty=d.editor->toPlainText(); const int undo=d.editor->document()->availableUndoSteps(); activate(d.child);
   choose("closeCurrentWindow",QMessageBox::Cancel); require(d.child&&d.child->isVisible()&&d.editor->toPlainText()==dirty&&d.editor->document()->availableUndoSteps()==undo&&d.editor->document()->isModified()&&read(d.path)==original.toUtf8(),"Close current Cancel lost dirty script text/history/disk identity"); invariant();
   choose("closeCurrentWindow",QMessageBox::Save); require(!d.child&&read(d.path)==dirty.toUtf8(),"Close current Save did not persist exactly the selected buffer"); invariant();
   auto &other=docs.last(); other.editor->appendPlainText("% discard only this edit"); activate(other.child); choose("closeCurrentWindow",QMessageBox::Discard); require(!other.child&&read(other.path)==other.text.toUtf8(),"Close current Discard changed the saved independent script"); invariant();
  } else if (mode=="DirtyCloseAll") {
   auto &d=docs.first(); d.editor->appendPlainText("% preserve dirty buffer Ω"); const auto dirty=d.editor->toPlainText(); const int undo=d.editor->document()->availableUndoSteps();
   auto *tree=window.findChild<QTreeWidget *>("Resources"); auto items=tree->findItems("Keep",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Pending resource fixture missing"); tree->itemDoubleClicked(items.first(),0); events();
   QPointer<QMdiSubWindow> pending=area->currentSubWindow(); auto *panel=dynamic_cast<EditablePanel *>(pending->widget()); auto *field=pending->findChild<QLineEdit *>("parameterValue"); require(panel&&field,"Pending resource form missing"); field->setText("12"); events(); require(panel->hasChanges(),"Pending resource draft not recorded");
   choose("closeAllWindows",QMessageBox::Cancel); require(d.child&&d.child->isVisible()&&d.editor->toPlainText()==dirty&&d.editor->document()->availableUndoSteps()==undo&&pending&&pending->isVisible()&&field->text()=="12"&&panel->hasChanges(),"Close all Cancel lost dirty script or unapplied resource draft"); invariant();
   choose("closeAllWindows",QMessageBox::Discard); require(visible().isEmpty()&&!pending&&!d.child&&read(d.path)==d.text.toUtf8(),"Close all Discard lost saved source or retained unapplied owners"); invariant();
  } else throw std::runtime_error("Unknown Window menu acceptance case");
  invariant(); require(window.runMission()==MainWindow::RunResult::Completed,"Window menu damaged executable active mission");
  require(QString::fromUtf8(read(output)).trimmed()=="9","Window menu mission differs from independent literal witness");
  save("values.txt",read(output)); save("observations.json",QJsonDocument(observations).toJson());
  const QJsonArray facets{"actual-window-menubar-and-popup-input","active-mission-source-model-history-protection","literal-mission-result","case-specific-geometry-navigation-or-close-oracle"};
  const QJsonObject proof{{"checkpoint","window-menu-"+mode},{"outcome","passed"},{"mode",mode},{"actions",actions},{"observations",observations},{"literal_result",9},{"facets",facets},{"scope","Actual Window menu geometry, cyclic navigation or close operation for three script windows, selected dirty Save/Discard/Cancel and unapplied resource close guards. No arbitrary window counts, all panel types, native OS input, platform/stress or final current-build qualification."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
