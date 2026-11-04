#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "Debugger.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QColorDialog>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QOpenGLWidget>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path);require(f.open(QIODevice::ReadOnly),"Plot-command artifact missing");return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Plot-command artifact write failed"); }
static const PlotCurve &curve(const PlotModel &model) {
   if(model.kind==PlotModel::Kind::XY) { require(model.curves.size()==1,"Plot-command XY curve count differs");return model.curves.first(); }
   for(const auto &c:model.curves) if(c.name=="Sat") return c;
   throw std::runtime_error("Plot-command spacecraft curve missing");
}
static QJsonArray samples(const PlotModel &model) {
   QJsonArray rows;for(const auto &p:curve(model).points) rows.append(QJsonArray{p.x,p.y,p.z,p.epoch,p.connect,double(p.frame)});return rows;
}
static bool ink(const QColor &actual,const QColor &wanted) {
   return std::abs(actual.red()-wanted.red())<35&&std::abs(actual.green()-wanted.green())<35&&std::abs(actual.blue()-wanted.blue())<35;
}
static int pixels(const QImage &image,const QColor &color,int left,int right) {
   int count=0;for(int y=image.height()/8;y<7*image.height()/8;++y) for(int x=std::max(left,image.width()/10);x<std::min(right,9*image.width()/10);++x) if(ink(image.pixelColor(x,y),color)) ++count;return count;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaPlotCommandResults");
   try {
      require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and plot-command case");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Plot-command fixture unavailable");
      const QString mode=argv[2];const bool clear=mode=="ClearXY",gl=mode=="PenOrbitGround";require(clear||gl||mode=="PenXY","Unknown plot-command case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"),report=files.filePath("plot results Δ.csv"),saved=files.filePath("plot commands Ω.script");
      auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),bytes);};
      MainWindow window;require(window.initialize(startup),"Plot-command initialization failed");window.show();events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Mission");require(source&&messages&&area&&tree,"Plot-command workspace missing");
      const double radius=7000,mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant(),speed=std::sqrt(mu/radius),omega=std::sqrt(mu/(radius*radius*radius));
      QString initial="% Plot-command witness Δ\nCreate Variable Keep;\nKeep = 41;\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(speed,'g',17)+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nP.Accuracy = 1e-12;\n";
      for(const auto &name:QStringList{"Trace","Mirror","Control","Idle"}) initial+="Create XYPlot "+name+";\n"+name+".XVariable = Sat.ElapsedSecs;\n"+name+".YVariables = {Sat.ElapsedSecs};\n"+name+".ShowPlot = true;\n";
      if(gl) initial+="Create OrbitView Orb;\nOrb.Add = {Sat};\nOrb.CoordinateSystem = EarthMJ2000Eq;\nOrb.ViewPointReference = Earth;\nOrb.ViewPointVector = [0 0 20000];\nOrb.ViewDirection = Earth;\nOrb.ShowPlot = true;\nCreate GroundTrack Map;\nMap.Add = {Sat};\nMap.ShowPlot = true;\n";
      const QString fields="Sat.ElapsedSecs Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ Keep";
      initial+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\n";
      if(clear) initial+="Report 'Before clear' Values "+fields+";\nClearPlot 'Erase' Idle; % retain erase\nReport 'Empty' Values "+fields+";\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nReport 'Renewed' Values "+fields+";\n";
      else initial+="PenUp 'Lift' Idle; % retain lift\nReport 'Before gap' Values "+fields+";\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nReport 'Gap' Values "+fields+";\nPenDown 'Lower' Idle; % retain lower\nPropagate P(Sat) {Sat.ElapsedSecs = 20};\nReport 'Resumed' Values "+fields+";\n";
      initial+="% Preserve tail Ω\n";source->setPlainText(initial);require(window.buildScript(),"Plot-command initial build failed");source->document()->setModified(false);save("initial.script",initial.toUtf8());
      const QStringList targets=gl?QStringList{"Map","Orb"}:QStringList{"Mirror","Trace"};QString wanted=initial;QJsonArray rejected,edits;
      auto configured=[&]{QJsonArray state;auto *sat=Moderator::Instance()->GetConfiguredObject("Sat");require(sat,"Plot-command configured spacecraft missing");for(const char *axis:{"X","Y","Z","VX","VY","VZ"})state.append(sat->GetRealParameter(axis));state.append(Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value"));return state;};const auto base=configured();
      auto open=[&](const QString &label)->QMdiSubWindow* {
         for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);
         auto snap=window.missionSnapshot();int index=-1;for(int i=0;i<snap.nodes.size();++i)if(missionCommandLabel(snap.nodes[i].statement).name==label)index=i;require(index>=0,"Plot-command named node absent");QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)item=*it;require(item,"Plot-command tree item absent");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();acceptanceView(window);tree->itemDoubleClicked(item,0);events();auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"Plot-command editor missing");child->showNormal();child->setGeometry(12,12,std::min(960,area->viewport()->width()-24),std::min(680,area->viewport()->height()-24));events();acceptanceView(*child);return child;
      };
      for(const auto &label:clear?QStringList{"Erase"}:QStringList{"Lift","Lower"}) {
         QPointer<QMdiSubWindow> child=open(label);auto *panel=dynamic_cast<CommandEditor *>(child->widget());const auto before=wanted;const int undo=source->document()->availableUndoSteps();const bool modified=source->document()->isModified();
         auto pristine=[&]{return source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified&&window.missionSnapshot().sourceScript==before&&configured()==base;};
         auto field=[&]{auto *f=panel->findChild<QLineEdit *>("commandField_Subscribers");require(f&&f->isVisible()&&f->isEnabled(),"Plot-command typed subscriber field unreachable");return f;};
         auto click=[&](QDialogButtonBox::StandardButton button){auto *box=panel->findChild<QDialogButtonBox *>();require(box&&box->button(button)&&box->button(button)->isEnabled(),"Plot-command action missing");acceptanceView(*child);QTest::mouseClick(box->button(button),Qt::LeftButton);events();};
         auto choose=[&]{std::exception_ptr error;bool visited=false;QTimer::singleShot(0,panel,[&]{auto *dialog=panel->findChild<QDialog *>("toggleSubscriberDialog");try{require(dialog,"Plot-command checklist missing");auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList");require(list,"Plot-command checklist entries missing");QStringList available;for(int i=0;i<list->count();++i){auto *item=list->item(i);available<<item->text();item->setCheckState(targets.contains(item->text())?Qt::Checked:Qt::Unchecked);}require(!available.contains("Values")&&available.contains("Trace")&&available.contains("Mirror"),"Plot-command picker includes nonplot or loses XY choices");if(gl)require(available.contains("Orb")&&available.contains("Map"),"Pen picker excludes Orbit/Ground");acceptanceView(*dialog);visited=true;QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok),Qt::LeftButton);}catch(...){error=std::current_exception();if(dialog)dialog->reject();}});auto *button=panel->findChild<QPushButton *>("commandChoose_Subscribers");require(button&&button->isVisible()&&button->isEnabled(),"Plot-command checklist action unreachable");QTest::mouseClick(button,Qt::LeftButton);events();if(error)std::rethrow_exception(error);require(visited&&field()->text().split(' ',Qt::SkipEmptyParts)==targets,"Plot-command selected subscriber order differs");};
         choose();require(panel->hasChanges()&&pristine(),"Plot-command pending picker changed mission/model/history");click(QDialogButtonBox::Cancel);require(!child&&pristine(),"Plot-command Cancel changed mission/model/history");child=open(label);panel=dynamic_cast<CommandEditor *>(child->widget());
         for(auto action:{QDialogButtonBox::Apply,QDialogButtonBox::Ok}){field()->setText("AbsentNASA");events();QPointer<CommandEditor> owner=panel;click(action);auto *status=owner?owner->findChild<QLabel *>("commandStatus"):nullptr;save("rejection-diagnostic.json",QJsonDocument(QJsonObject{{"source",source->toPlainText()},{"messages",messages->toPlainText()}}).toJson());require(owner&&child&&owner==panel&&panel->hasChanges()&&pristine()&&field()->text()=="AbsentNASA"&&status&&!status->text().isEmpty(),"Plot-command invalid Apply/OK lost atomic source/model/history/draft/owner");rejected.append(QJsonObject{{"command",label},{"action",action==QDialogButtonBox::Apply?"Apply":"OK"},{"diagnostic",status->text()}});}
         choose();const auto command=clear?QString("ClearPlot"):label=="Lift"?QString("PenUp"):QString("PenDown");const auto oldText=command+" '"+label+"' Idle;",newText=command+" '"+label+"' "+targets.join(' ')+";";require(wanted.count(oldText)==1,"Plot-command exact replacement target ambiguous");wanted.replace(oldText,newText);click(QDialogButtonBox::Apply);panel=dynamic_cast<CommandEditor *>(child->widget());save("apply-diagnostic.json",QJsonDocument(QJsonObject{{"source",source->toPlainText()},{"messages",messages->toPlainText()}}).toJson());require(panel&&!panel->hasChanges()&&source->toPlainText()==wanted,"Plot-command Apply did not commit exact source");const int afterUndo=source->document()->availableUndoSteps();click(QDialogButtonBox::Apply);require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==afterUndo,"Plot-command repeated Apply changed history");click(QDialogButtonBox::Close);require(!child,"Plot-command clean close retained owner");source->undo();require(source->toPlainText()==before&&window.buildScript()&&configured()==base,"Plot-command Undo failed exact restoration");source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"Plot-command Redo failed");edits.append(QJsonObject{{"label",label},{"subscribers",targets.join(' ')}});
      }
      require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"Plot-command Unicode save/reopen failed");save("mission.script",wanted.toUtf8());save("rejections.json",QJsonDocument(rejected).toJson());
      for(const auto &label:clear?QStringList{"Erase"}:QStringList{"Lift","Lower"}){auto *child=open(label);auto *panel=dynamic_cast<CommandEditor *>(child->widget());require(panel->findChild<QLineEdit *>("commandField_Subscribers")->text()==targets.join(' '),"Plot-command reopened typed subscribers differ");auto *box=panel->findChild<QDialogButtonBox *>();QTest::mouseClick(box->button(QDialogButtonBox::Ok),Qt::LeftButton);events();require(!area->subWindowList().contains(child),"Plot-command clean OK retained owner");}
      const int runUndo=source->document()->availableUndoSteps();QSet<int> breaks;auto snap=window.missionSnapshot();for(int i=0;i<snap.nodes.size();++i)if(snap.nodes[i].type=="Report"){breaks.insert(i);require(window.setBreakpoint(i,true),"Plot-command breakpoint unavailable");}require(breaks.size()==3,"Plot-command phase boundaries missing");Debugger *debug=nullptr;for(auto *object:window.children())if(auto *d=dynamic_cast<Debugger *>(object))debug=d;require(debug,"Plot-command debugger missing");
      QJsonArray phases;int calls=0;QTimer timer,timeout;timer.setInterval(10);timeout.setSingleShot(true);timeout.setInterval(60000);std::exception_ptr failure;
      auto capture=[&](const QString &name,const QString &phase){auto model=window.plotReceiver()->model(name);require(bool(model),"Plot-command live model missing");require(window.plotReceiver()->show(name),"Plot-command live viewer missing");events();auto *child=area->activeSubWindow();auto *view=child?dynamic_cast<PlotWidget *>(child->widget()):nullptr;require(view,"Plot-command real plot widget missing");child->showNormal();child->setGeometry(0,0,std::min(960,area->viewport()->width()),std::min(700,area->viewport()->height()));area->setActiveSubWindow(child);child->raise();
         if(model->kind==PlotModel::Kind::XY&&curve(*model).color!=QColor(Qt::red)) {
            std::exception_ptr styleFailure;bool styled=false;
            QTimer::singleShot(0,view,[&]{auto *dialog=view->findChild<QDialog *>("plotStyleDialog");try{
               require(dialog,"Plot-command style fixture dialog missing");dialog->findChild<QTabWidget *>("plotStyleTabs")->setCurrentIndex(3);events();auto *page=dialog->findChild<QWidget *>("curveStyle_0");require(page&&page->isVisible(),"Plot-command style fixture curve page missing");page->findChild<QSpinBox *>("Line width")->setValue(3);acceptanceView(*dialog);
               std::exception_ptr pickerFailure;bool picked=false;QTimer::singleShot(0,dialog,[&]{auto *picker=qobject_cast<QColorDialog *>(QApplication::activeModalWidget());try{require(picker,"Plot-command color fixture picker missing");picker->setCurrentColor(Qt::red);acceptanceView(*picker);picked=true;picker->accept();}catch(...){pickerFailure=std::current_exception();if(picker)picker->reject();}});
               QTest::mouseClick(page->findChild<QPushButton *>("Curve color"),Qt::LeftButton);if(pickerFailure)std::rethrow_exception(pickerFailure);require(picked,"Plot-command red curve fixture was not selected");QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok),Qt::LeftButton);styled=true;
            }catch(...){styleFailure=std::current_exception();if(dialog)dialog->reject();}});
            view->findChild<QAction *>("plotStyleAction")->trigger();events();if(styleFailure)std::rethrow_exception(styleFailure);require(styled&&curve(*model).color==QColor(Qt::red),"Plot-command red curve fixture did not reach actual output");
         }
         view->refresh();events();acceptanceView(*child);auto *canvas=view->canvas();if(model->kind==PlotModel::Kind::Orbit&&qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible"){auto *native=canvas->findChild<QOpenGLWidget *>();require(native&&native->isValid(),"Plot-command visible native orbit renderer invalid");}const auto image=canvas->captureImage();require(!image.isNull()&&image.width()>300&&image.height()>180,"Plot-command rendered image missing");if(!evidence.isEmpty())require(image.save(QDir(evidence).filePath(phase+"-"+name+".png")),"Plot-command visual-oracle image write failed");return image;};
      QObject::connect(&timer,&QTimer::timeout,&window,[&]{if(!debug->isWaiting())return;timer.stop();try{
         const double time=clear?(calls<2?20:40):20*(calls+1);const QString phase="phase"+QString::number(calls);auto control=window.plotReceiver()->model("Control");require(control&&curve(*control).points.size()>=3,"Unselected control plot missing");const auto controlImage=capture("Control",phase);const auto color=curve(*control).color;require(color!=Qt::white&&color!=Qt::black,"Plot-command curve has no discriminable color");QJsonObject observation{{"phase",calls},{"elapsed",time},{"control",samples(*control)}};QJsonObject outputs;
         for(const auto &name:targets){auto model=window.plotReceiver()->model(name);require(bool(model),"Selected plot missing at boundary");const auto &points=curve(*model).points;const auto raw=samples(*model);int suppressed=0,connected=0;double last=-1;
            if(clear&&calls==1)require(points.empty(),"ClearPlot did not immediately erase every selected curve");else{require(points.size()>=3,"Plot-command accepted samples missing");for(const auto &point:points){const double t=model->kind==PlotModel::Kind::XY?point.x:(point.epoch-21545)*86400.;require(std::isfinite(t)&&t>=last-1e-5,"Plot-command sample times invalid or reversed");if(model->kind==PlotModel::Kind::XY)require(std::abs(point.x-point.y)<1e-10,"XY independent elapsed-time diagonal differs");if(clear&&calls==2)require(t>=20-1e-5,"ClearPlot retained pre-clear history");if(!clear&&t>20+1e-5&&t<=40+1e-5){require(!point.connect,"PenUp drew a line across the disabled interval");++suppressed;}if(!clear&&t>40+1e-5&&point.connect)++connected;last=t;}require(std::abs(last-time)<1e-4,"Plot-command final sample time differs");if(!clear&&calls>0)require(suppressed>0,"PenUp disabled interval was not exercised");if(!clear&&calls==2)require((model->kind!=PlotModel::Kind::XY||model->penDown)&&connected>0,"PenDown did not resume connected samples");if(!clear&&calls<2&&model->kind==PlotModel::Kind::XY)require(!model->penDown,"PenUp state did not reach real output model");}
            const auto image=capture(name,phase);if(model->kind==PlotModel::Kind::XY){const int total=pixels(image,color,0,image.width());if(clear&&calls==1)require(total==0,"ClearPlot left rendered curve pixels");else require(total>5,"Plot-command live XY curve pixels missing");if(!clear&&calls>0){require(image.size()==controlImage.size(),"Pixel oracle plots have different dimensions");int left=image.width(),right=-1;for(int y=controlImage.height()/8;y<7*controlImage.height()/8;++y)for(int x=controlImage.width()/10;x<9*controlImage.width()/10;++x)if(ink(controlImage.pixelColor(x,y),color)){left=std::min(left,x);right=std::max(right,x);}require(right-left>100,"Independent control-curve pixel calibration missing");const double a=calls==1?.60:.40,b=calls==1?.85:.60;const int low=left+int((right-left)*a),high=left+int((right-left)*b);const int targetInk=pixels(image,color,low,high),controlInk=pixels(controlImage,color,low,high);require(controlInk>5&&targetInk==0,"PenUp rendered data inside independently calibrated disabled interval");outputs[name+"-pixel-witness"]=QJsonObject{{"control_gap_pixels",controlInk},{"target_gap_pixels",targetInk},{"left",low},{"right",high}};}}
            require(samples(*model)==raw,"Plot display changed retained numerical/connectivity samples");outputs[name]=QJsonObject{{"points",raw},{"pen_down",model->penDown},{"history_generation",double(model->historyGeneration)},{"suppressed_samples",suppressed},{"resumed_edges",connected}};
         }
         const auto &controlPoints=curve(*control).points;double previous=-1;for(const auto &point:controlPoints){if(previous>=0&&point.x>previous+1e-5)require(point.connect,"Pen/Clear command affected unselected control output");require(std::abs(point.x-point.y)<1e-10,"Unselected independent XY diagonal differs");previous=point.x;}require(std::abs(previous-time)<1e-4,"Unselected control final time differs");observation["outputs"]=outputs;phases.append(observation);save("phases.json",QJsonDocument(phases).toJson());++calls;debug->resume();timer.start();
      }catch(...){failure=std::current_exception();window.stopMission();}});
      QObject::connect(&timeout,&QTimer::timeout,&window,[&]{failure=std::make_exception_ptr(std::runtime_error("Plot-command boundary observer timed out"));window.stopMission();});timer.start();timeout.start();messages->clear();const auto result=window.runMission();timer.stop();timeout.stop();save("messages.txt",messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);require(result==MainWindow::RunResult::Completed&&calls==3,"Plot-command mission or phase count failed");
      const auto raw=read(report);save("values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==3,"Plot-command independent report row count differs");QJsonArray rows;for(int i=0;i<3;++i){auto columns=lines[i].split(',');require(columns.size()==8,"Plot-command independent report columns differ");const double t=clear?(i<2?20:40):20*(i+1),angle=omega*t;const QList<double> expected={t,radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0,41};QJsonArray row;for(int j=0;j<8;++j){bool ok=false;double v=columns[j].trimmed().toDouble(&ok);require(ok&&std::isfinite(v)&&std::abs(v-expected[j])<(j==0?1e-5:2e-5),"Plot commands changed independent circular-state/elapsed/sentinel oracle");row.append(v);}rows.append(row);}
      require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==runUndo&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"Plot-command execution changed source/history or failed to return GUI control");
      const QJsonObject proof{{"checkpoint","plot-command-result"},{"outcome","passed"},{"case",mode},{"targets",QJsonArray::fromStringList(targets)},{"mu",mu},{"radius",radius},{"rows",rows},{"phase_count",calls},{"rejections",rejected},{"edits",edits},{"facets",QJsonArray{"actual-mission-tree-command-editor-and-subscriber-checklist","cancel-invalid-apply-ok-atomicity","exact-apply-idempotence-history-unicode-reopen","live-command-boundary-plot-samples-and-unselected-isolation","actual-rendered-plot-and-xy-pixel-oracles","independent-circular-state-and-time-witnesses"}},{"scope","Selected PenUp/PenDown XY and Orbit/Ground connected-line intervals, or immediate multi-XY ClearPlot and renewed output. XY rendered pixel gaps/clearing are checked independently against an unselected diagonal control. Orbit/Ground rendered output is captured with native exposure and connectivity assertions, without independent native-renderer pixel-gap qualification. Arbitrary markers/solver/propagation/frame/science/native-input/hardware/platform/stress and full-current-build qualification remain open."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
