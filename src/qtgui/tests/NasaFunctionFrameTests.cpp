#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "ReportViewer.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "Parameter.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
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
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static QByteArray read(const QString &path) {QFile file(path);require(file.open(QIODevice::ReadOnly),"Function control evidence missing");return file.readAll();}
static void write(const QString &path,const QByteArray &bytes) {QFile file(path);require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Function control evidence write failed");}
static double value(const char *name) {auto *object=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject(name));require(object,"Function runtime parameter unavailable");return object->EvaluateReal();}
static void reveal(QWidget *control) {
   require(control,"Function control widget missing");
   for (QWidget *page=control;page;page=page->parentWidget()) {
      for (auto *tabs:control->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(control);
   }
   events();require(control->isVisible() && control->isEnabled(),"Function control widget unreachable");
}
struct Fixture {QString accept,mission;QJsonArray summary,words;};
static Fixture fixture(const QString &mode,const QString &path)
{
   Fixture f;const QString head="Create GmatFunction Accept;\nAccept.FunctionPath = '"+path+"';\n";
   const bool control=mode=="NestedStop" || mode=="NestedPause",failure=mode=="Failure";
   QString outputs="[out]",declarations="Create Variable out next child;\n",setup,base,combine,callOutputs="[Result]",columns="Depth Result Count";
   if (mode=="Array") {
      declarations="Create Variable next;\nCreate Array out[2,2] marker[2,2] child[2,2];\n";
      setup="marker(1,1) = depth + 1;\nmarker(1,2) = -2 * depth;\nmarker(2,1) = depth * depth;\nmarker(2,2) = 5 - depth;\n";base="out = marker;\n";combine="out = marker + child;\n";callOutputs="[OutA]";columns="Depth OutA(1,1) OutA(1,2) OutA(2,1) OutA(2,2) Count";
   } else if (mode=="String") {
      declarations="Create Variable next;\nCreate String out marker child;\n";
      setup="If depth == 0\nmarker = 'base';\nElse\nIf depth == 1\nmarker = 'one';\nElse\nIf depth == 2\nmarker = 'two';\nElse\nmarker = 'three';\nEndIf;\nEndIf;\nEndIf;\n";base="out = marker;\n";combine="out = strcat(marker, '/', child);\n";callOutputs="[ResultText]";columns="Depth Count";
   } else if (mode=="Spacecraft" || mode=="Coordinate" || mode=="Defaults" || mode=="Automatic") {
      const bool automatic=mode=="Automatic";
      declarations=automatic ? "Create Spacecraft local;\nCreate Variable out energy next child childEnergy;\n" : "Create Spacecraft local out child;\nCreate Variable next;\n";
      setup="local.CoordinateSystem = SharedFrame;\nlocal.DateFormat = A1ModJulian;\nlocal.Epoch = '21545';\nlocal.DisplayStateType = Cartesian;\nlocal.X = 1000 + 10 * depth;\n";
      if (automatic) {setup+="local.Y = -200;\nlocal.Z = 300;\nlocal.VX = 1;\nlocal.VY = 2;\nlocal.VZ = 3;\n";outputs="[out, energy]";base="out = local.Earth.RMAG;\nenergy = local.Earth.C3Energy;\n";combine="out = local.Earth.RMAG + child;\nenergy = local.Earth.C3Energy + childEnergy;\n";callOutputs="[Result, ResultEnergy]";columns="Depth Result ResultEnergy Count";}
      else {setup+="local.Y = -200 + depth;\nlocal.Z = 300 - depth;\nlocal.VX = 1.25 + 0.1 * depth;\nlocal.VY = -0.5;\nlocal.VZ = 0.75;\nlocal.DryMass = 300 + depth;\n";base="out = local;\n";combine="out = local;\n";for (const auto &field:QStringList{"X","Y","Z","VX","VY","VZ","DryMass"}) combine+="out."+field+" = local."+field+" + child."+field+";\n";callOutputs="[Returned]";columns="Depth Returned.X Returned.Y Returned.Z Returned.VX Returned.VY Returned.VZ Returned.DryMass Count";if (mode=="Coordinate" || mode=="Defaults") columns="Depth Returned.EarthMJ2000Eq.X Returned.EarthMJ2000Eq.Y Returned.EarthMJ2000Eq.Z Returned.EarthMJ2000Eq.VX Returned.EarthMJ2000Eq.VY Returned.EarthMJ2000Eq.VZ Returned.DryMass Count";}
   } else if (failure) {base="out = sqrt(Radicand);\n";combine="out = child + depth;\n";}
   else {require(control,"Unknown function-frame case");declarations="Create Variable out next child k;\n";base="out = 0;\nFor k = 1:Limit\nProgress = Progress + 1;\nout = out + 1;\nEndFor;\n";combine="out = child + depth;\n";columns="Depth Result Count Progress";}
   if (mode=="Coordinate") {
      // NASA calculation-parameter defaults differ on reads and writes. Use
      // explicit dependencies here; Defaults separately tests that asymmetry.
      for (const auto &field:QStringList{"X","Y","Z","VX","VY","VZ"}) {
         setup.replace("local."+field+" =", "local.SharedFrame."+field+" =");
         combine.replace("out."+field+" = local."+field+" + child."+field,
                         "out.SharedFrame."+field+" = local.SharedFrame."+field+" + child.SharedFrame."+field);
      }
   }
   const QString childOutputs=mode=="Automatic" ? "[child, childEnergy]":"[child]";
   f.accept="function "+outputs+" = Accept(depth)\n"+head+declarations+"Global Count Progress Limit Radicand;\nBeginMissionSequence;\nCount = Count + 1;\n"+setup+"If depth <= 0\n"+base+"Else\nnext = depth - 1;\n"+childOutputs+" = Accept(next);\n"+combine+"EndIf;\n% Recursive data-frame GUI acceptance Δ\n";
   int count=0;
   for (int d:(control || failure ? QList<int>{3}:mode=="Defaults" ? QList<int>{0,1,3,2}:QList<int>{3,2})) {
      f.mission+="Depth = "+QString::number(d)+";\n'"+QString(count==0 ? "Before":"Next")+"' "+callOutputs+" = Accept(Depth);\nReport Summary "+columns+";\n";
      if (mode=="String") f.mission+="Report Words ResultText;\n";
      count+=d+1;f.summary.append(d);
      if (mode=="Array") {double a=0,b=0,c=0,e=0;for (int k=0;k<=d;++k) {a+=k+1;b-=2*k;c+=k*k;e+=5-k;}for (double x:{a,b,c,e}) f.summary.append(x);}
      else if (mode=="String") f.words.append(d==3 ? "three/two/one/base":"two/one/base");
      else if (mode=="Spacecraft" || mode=="Coordinate" || mode=="Defaults") {
         const double n=d+1,sum=d*(d+1)/2.;std::array<double,7> state{1000*n+10*sum,-200*n+sum,300*n-sum,1.25*n+.1*sum,-.5*n,.75*n,300*n+sum};
         const double e=84381.448/3600*std::acos(-1.0)/180;
         auto rotate=[&](std::array<double,7> &v) {for (int offset:{0,3}) {const double y=v[offset+1],z=v[offset+2];v[offset+1]=std::cos(e)*y-std::sin(e)*z;v[offset+2]=std::sin(e)*y+std::cos(e)*z;}};
         if (mode=="Defaults") {
            state.fill(0);
            for (int k=0;k<=d;++k) {const std::array<double,7> local{1000.+10*k,-200.+k,300.-k,1.25+.1*k,-.5,.75,300.+k};for (int j=0;j<7;++j) state[j]+=local[j];if (k>0) rotate(state);}
         }
         if (mode=="Coordinate" || mode=="Defaults") rotate(state);
         for (auto x:state) f.summary.append(x);
      }
      else if (mode=="Automatic") {double rsum=0,esum=0;for (int k=0;k<=d;++k) {const double r=std::sqrt(std::pow(1000+10*k,2)+200*200+300*300);rsum+=r;esum+=14-800000/r;}f.summary.append(rsum);f.summary.append(esum);}
      else f.summary.append(failure ? 8:control && mode=="NestedStop" ? 43:40006);
      f.summary.append(count);if (control) f.summary.append(mode=="NestedStop" ? 37:40000);
   }
   return f;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaFunctionFrames");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and function-frame case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Function-frame directory unavailable");
      const QString mode=argv[2];const bool control=mode=="NestedStop" || mode=="NestedPause",stop=mode=="NestedStop",failing=mode=="Failure";
      const auto folder=files.filePath("recursive frames Δ");require(QDir().mkpath(folder),"Function-frame Unicode folder failed");const auto accept=QDir(folder).filePath("Accept.gmf"),summary=files.filePath("Summary Δ.txt"),words=files.filePath("Words Δ.txt"),path=files.filePath("Frames Δ.script");const auto f=fixture(mode,accept);
      const QString originalBody=f.accept+"% Initial external revision\n";write(accept,originalBody.toUtf8());
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      QString initial="% Retain recursive frame acceptance Δ\nEarth.Mu = 400000; % known synthetic model input\nCreate CoordinateSystem SharedFrame;\nSharedFrame.Origin = Earth;\nSharedFrame.Axes = MJ2000Eq;\nCreate GmatFunction Accept;\nAccept.FunctionPath = '"+accept+"';\nCreate Variable Depth Result ResultEnergy Count Progress Limit Radicand Keep;\nResult = -99;\nResultEnergy = -99;\nCount = 0;\nProgress = 0;\nLimit = 40000; % bounded function input\nRadicand = -1; % runtime domain input\nKeep = 41; % preserve initializer\nCreate Array OutA[2,2];\nCreate String ResultText;\nCreate Spacecraft Returned;\nReturned.CoordinateSystem = SharedFrame;\nReturned.DateFormat = A1ModJulian;\nReturned.Epoch = '21545';\nCreate ReportFile Summary Words;\nSummary.Filename = '"+summary+"';\nWords.Filename = '"+words+"';\nSummary.WriteHeaders = false;\nWords.WriteHeaders = false;\nSummary.Precision = 16;\nBeginMissionSequence;\nGlobal Count Progress Limit Radicand;\n"+f.mission+"Keep = Keep + 1; % preserve mission\n";
      if (mode=="Coordinate" || mode=="Defaults") initial.replace("SharedFrame.Axes = MJ2000Eq;","SharedFrame.Axes = MJ2000Ec;");
      const auto destination=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &bytes) {if (!destination.isEmpty()) write(QDir(destination).filePath(name),bytes);};save("initial.script",initial.toUtf8());save("accept-initial.gmf",originalBody.toUtf8());
      MainWindow window;require(window.initialize(startup),"Function-control initialization failed");window.show();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *resources=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(source && messages && resources && area,"Function-control workspace missing");source->setPlainText(initial);const bool built=window.buildScript();if (!built) std::cerr<<messages->toPlainText().toStdString();require(built,"Function-control baseline failed");const int initialUndo=source->document()->availableUndoSteps();
      auto view=[&](QMdiSubWindow *child) {if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") {acceptanceView(window);child->showNormal();events();child->move(0,0);child->resize(area->viewport()->size());area->setActiveSubWindow(child);child->raise();events();}acceptanceView(*child);};
      auto openResource=[&](const QString &name) {const auto items=resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Function-control resource tree item missing");resources->itemDoubleClicked(items.front(),0);events();auto *child=area->activeSubWindow();require(child && dynamic_cast<ResourceEditor *>(child->widget()),"Function-control actual resource editor missing");view(child);return child;};
      auto *child=openResource("Accept");auto *resource=dynamic_cast<ResourceEditor *>(child->widget());auto *edit=resource->findChild<QPushButton *>("editFunctionFile");reveal(edit);int fileCancels=0;
      for (bool commit:{false,true}) {
         acceptanceView(*child);std::exception_ptr error;
         QTimer::singleShot(0,resource,[&] {auto *dialog=resource->findChild<QDialog *>("functionFileDialog");try {require(dialog,"Function-control actual file editor missing");auto *text=dialog->findChild<QPlainTextEdit *>("functionFileText");require(text && text->toPlainText()==originalBody,"Function-control editor read wrong file");text->setPlainText(f.accept);text->document()->setModified(true);acceptanceView(*dialog);require(source->toPlainText()==initial && read(accept)==originalBody.toUtf8() && source->document()->availableUndoSteps()==initialUndo,"Function file draft escaped Save");dialog->findChild<QDialogButtonBox *>()->button(commit ? QDialogButtonBox::Save:QDialogButtonBox::Cancel)->click();require(dialog->result()==(commit ? QDialog::Accepted:QDialog::Rejected),"Function-control file Save/Cancel failed");if (!commit) ++fileCancels;}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});
         edit->click();if (error) std::rethrow_exception(error);require(read(accept)==(commit ? f.accept:originalBody).toUtf8() && source->toPlainText()==initial && !resource->hasChanges(),"Function file Save/Cancel changed mission/path");
      }
      require(child->close(),"Function-control clean resource panel did not close");events();
      const auto snapshot=window.missionSnapshot();int index=-1;for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].statement.contains("'Before'")) index=i;require(index>=0 && snapshot.nodes[index].editable,"Function-control call source mapping unavailable");auto *tree=window.findChild<QTreeWidget *>("Mission");QTreeWidgetItemIterator it(tree);while (*it && (!(*it)->data(0,Qt::UserRole).isValid() || (*it)->data(0,Qt::UserRole).toInt()!=index)) ++it;require(*it,"Function-control mission item missing");tree->itemDoubleClicked(*it,0);events();child=area->activeSubWindow();auto *panel=dynamic_cast<CommandEditor *>(child->widget());require(panel,"Function-control actual command panel missing");view(child);auto *name=panel->findChild<QLineEdit *>("commandName");reveal(name);name->setText("Acceptance call");require(source->toPlainText()==initial && panel->hasChanges(),"Function-control command draft escaped Apply");acceptanceView(*child);panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CommandEditor *>(child->widget());if (panel && panel->hasChanges()) std::cerr<<panel->findChild<QLabel *>("commandStatus")->text().toStdString();require(panel && !panel->hasChanges(),"Function-control command Apply failed");const auto applied=source->toPlainText();require(applied==QString(initial).replace("'Before'","'Acceptance call'"),"Function-control Apply changed unrelated source");require(child->close(),"Function-control clean command did not close");events();source->undo();require(source->toPlainText()==initial && window.buildScript() && read(accept)==f.accept.toUtf8(),"Function-control exact Undo/file isolation failed");source->redo();require(source->toPlainText()==applied && window.buildScript(),"Function-control exact Redo failed");require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && source->toPlainText()==applied,"Function-control Unicode persistence failed");
      save("mission.script",source->toPlainText().toUtf8());save("accept.gmf",read(accept));
      QJsonArray observations,reports;int requested=0,held=0,phase=0;double maximum=0;std::exception_ptr failure;QTimer timer,deadline;timer.setInterval(5);deadline.setSingleShot(true);deadline.setInterval(15000);QElapsedTimer holdClock;
      auto *pauseAction=window.findChild<QAction *>("pauseMission"),*stopAction=window.findChild<QAction *>("stopMission"),*runAction=window.findChild<QAction *>("runMission");require(pauseAction && stopAction && runAction,"Recursive GUI run actions missing");
      if (control) {
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            if (!window.isRunning() || phase!=0) return;auto *p=dynamic_cast<Parameter *>(Moderator::Instance()->GetInternalObject("Progress"));if (!p) return;const double current=p->EvaluateReal();if (!(current>0 && current<40000)) return;
            timer.stop();try {require(value("Count")==4 && source->isReadOnly() && !resources->isEnabled(),"GUI action not inside the deepest recursive call");requested=current;phase=1;
               if (stop) {stopAction->trigger();require(Moderator::Instance()->GetRunState()==Gmat::IDLE && !stopAction->isEnabled(),"Recursive Stop action did not take effect");observations.append(QJsonObject{{"action","stop"},{"progress",current},{"count",value("Count")}});acceptanceView(window);}
               else {pauseAction->trigger();require(Moderator::Instance()->GetRunState()==Gmat::PAUSED && runAction->isEnabled() && !pauseAction->isEnabled(),"Recursive Pause action did not take effect");holdClock.start();observations.append(QJsonObject{{"action","pause"},{"progress",current},{"count",value("Count")}});acceptanceView(window);QTimer::singleShot(150,&window,[&] {try {held=value("Progress");require(holdClock.elapsed()>=100 && held==requested && value("Count")==4 && window.isRunning() && Moderator::Instance()->GetRunState()==Gmat::PAUSED,"Recursive function advanced while paused");observations.append(QJsonObject{{"action","held"},{"progress",held},{"count",value("Count")},{"elapsed_ms",int(holdClock.elapsed())}});acceptanceView(window);runAction->trigger();require(Moderator::Instance()->GetRunState()==Gmat::RUNNING,"Recursive Resume action failed");observations.append(QJsonObject{{"action","resume"},{"progress",held}});phase=2;}catch (...) {failure=std::current_exception();window.stopMission();}});}
            }catch (...) {failure=std::current_exception();window.stopMission();}
         });
         QObject::connect(&deadline,&QTimer::timeout,&window,[&] {failure=std::make_exception_ptr(std::runtime_error("Recursive GUI control did not release execution"));window.stopMission();});timer.start();deadline.start();
      }
      messages->clear();const auto outcome=window.runMission();timer.stop();deadline.stop();events();save("messages.txt",messages->toPlainText().toUtf8());if (failure) std::rethrow_exception(failure);require(!window.isRunning() && !source->isReadOnly() && resources->isEnabled(),"Recursive execution did not release the GUI");
      const bool interrupted=failing || stop;
      if (interrupted) {
         require(outcome==(failing ? MainWindow::RunResult::Failed:MainWindow::RunResult::Stopped),"Recursive failure/stop returned the wrong outcome");require(value("Count")==4 && value("Keep")==41 && value("Depth")==3 && value("Result")==-99,"Recursive unwinding committed an output or continued the main mission");
         if (failing) require(messages->toPlainText().contains("Sqrt(x) x is less than 0.0"),"Recursive math-domain error lacks its exact diagnostic");
         else require(requested>0 && requested<40000 && value("Progress")>=requested && value("Progress")<40000,"Stop was not processed inside recursion");
         observations.append(QJsonObject{{"action","finished"},{"result",failing ? "failed":"stopped"},{"progress",value("Progress")},{"count",value("Count")},{"keep",value("Keep")},{"output",value("Result")}});messages->verticalScrollBar()->setValue(messages->verticalScrollBar()->maximum());acceptanceView(window);
      } else {if (outcome!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(outcome==MainWindow::RunResult::Completed && value("Keep")==42,"Recursive data mission failed");if (control) {require(requested>0 && phase==2 && held==requested && value("Progress")==40000 && value("Count")==4,"Nested pause/resume result differs");observations.append(QJsonObject{{"action","finished"},{"result","completed"},{"progress",value("Progress")},{"count",value("Count")},{"keep",value("Keep")},{"output",value("Result")}});}}
      QString returnedFrame;
      if (mode=="Coordinate" || mode=="Defaults") {auto *object=Moderator::Instance()->GetInternalObject("Returned");require(object,"Returned coordinate-scope spacecraft unavailable");returnedFrame=QString::fromStdString(object->GetStringParameter("CoordinateSystem"));require(returnedFrame=="SharedFrame" && initial.contains("SharedFrame.Axes = MJ2000Ec;") && !f.accept.contains("Global SharedFrame"),"Function did not retain its implicitly global nonidentity frame");}
      require(source->toPlainText()==applied,"Recursive execution changed main source");
      if (interrupted) {
         const QString target=failing ? "Radicand":"Limit",replacement=failing ? "4":"37";child=openResource(target);resource=dynamic_cast<ResourceEditor *>(child->widget());auto *field=resource->findChild<QLineEdit *>("parameterValue");reveal(field);field->setText(replacement);require(source->toPlainText()==applied && resource->hasChanges(),"Recursive recovery input escaped parent Apply");acceptanceView(*child);resource->requestApply();events();resource=dynamic_cast<ResourceEditor *>(child->widget());require(resource && !resource->hasChanges(),"Recursive recovery input Apply failed");const auto corrected=source->toPlainText();require(corrected!=applied && child->close(),"Recursive correction did not close cleanly");events();source->undo();require(source->toPlainText()==applied && window.buildScript() && read(accept)==f.accept.toUtf8(),"Recursive recovery input Undo failed");source->redo();require(source->toPlainText()==corrected && window.buildScript(),"Recursive recovery input Redo failed");require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript() && source->toPlainText()==corrected,"Recursive recovery Unicode persistence failed");save("recovered.script",source->toPlainText().toUtf8());
         messages->clear();const auto recovered=window.runMission();events();save("recovery-messages.txt",messages->toPlainText().toUtf8());if (recovered!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString();require(recovered==MainWindow::RunResult::Completed && value("Keep")==42 && value("Count")==4,"Recursive failure/stop did not recover without restart");
      }
      auto checkReport=[&](const QString &name,const QString &filename,const QJsonArray &expected) {
         const auto raw=read(filename);save(name+".txt",raw);const auto tokens=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(tokens.size()==expected.size(),"Recursive frame report cardinality differs");QJsonArray actual;
         for (int i=0;i<tokens.size();++i) {if (expected[i].isString()) {require(tokens[i]==expected[i].toString(),"Recursive string frame differs from independent literal-chain oracle");actual.append(tokens[i]);}else {bool ok=false;const double x=tokens[i].toDouble(&ok),error=std::abs(x-expected[i].toDouble());actual.append(x);maximum=std::max(maximum,error);if (!ok || error>1e-9) std::cerr<<"FRAME_DIFF "<<mode.toStdString()<<" component="<<i<<" actual="<<x<<" expected="<<expected[i].toDouble()<<" error="<<error<<std::endl;require(ok && std::isfinite(x) && error<=1e-9,"Recursive frame result differs from independent data/geometry oracle");}}
         auto *output=window.findChild<QTreeWidget *>("Output");const auto matches=output->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Recursive report tree item missing");output->itemDoubleClicked(matches.front(),0);events();QMdiSubWindow *viewer=nullptr;for (auto *w:area->subWindowList()) if (dynamic_cast<ReportViewer *>(w->widget())) viewer=w;require(viewer,"Recursive actual report viewer missing");view(viewer);auto *shown=viewer->findChild<QPlainTextEdit *>();require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(raw).trimmed(),"Recursive report viewer differs from checked bytes");shown->horizontalScrollBar()->setValue(shown->horizontalScrollBar()->maximum());events();acceptanceView(*viewer);require(viewer->close(),"Recursive report did not close");events();reports.append(QJsonObject{{"name",name},{"actual",actual},{"expected",expected}});
      };
      checkReport("Summary",summary,f.summary);if (!f.words.isEmpty()) checkReport("Words",words,f.words);
      const QJsonObject proof{{"checkpoint","function-frame-"+mode},{"outcome","passed"},{"mode",mode},{"file_cancels",fileCancels},{"returned_frame",returnedFrame},{"frame_obliquity_arcsec",(mode=="Coordinate" || mode=="Defaults") ? 84381.448:0},{"observations",observations},{"reports",reports},{"max_numeric_error",maximum},{"facets",QJsonArray{"actual-function-file-editor","exact-command-history-and-unicode","recursive-local-data-or-unwind","independent-data-and-geometry-results","actual-report-viewer"}},{"scope","Selected recursive array/string/spacecraft/automatic-reference samples at depths 3/2, deep scalar error/stop recovery and nested pause. Synthetic known mu and prescribed Earth frames support independent radius/C3 and fixed-rotation algebraic checks; no empirical ephemeris/orientation/time accuracy. Other shapes/depths/resources, type changes/local subscribers, native OS input/platform/stress/current private NASA corpus and full regression remain open."}};std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}return 1;
}
