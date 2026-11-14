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
#include <QCheckBox>
#include <QScreen>
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
static QByteArray read(const QString &path) { QFile f(path);require(f.open(QIODevice::ReadOnly),"MarkPoint artifact missing");return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"MarkPoint artifact write failed"); }
static const PlotCurve &curve(const PlotModel &model) {
   if(model.kind==PlotModel::Kind::XY) { require(model.curves.size()==1,"MarkPoint XY curve count differs");return model.curves.first(); }
   for(const auto &c:model.curves) if(c.name=="Sat") return c;
   throw std::runtime_error("MarkPoint spacecraft curve missing");
}
static QJsonArray samples(const PlotModel &model) {
   QJsonArray rows;for(const auto &p:curve(model).points) rows.append(QJsonArray{p.x,p.y,p.z,p.epoch,p.connect,double(p.frame),p.highlighted});return rows;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaMarkPoint");
   try {
      require(argc==2&&acceptanceWidgetPlatform()&&QGuiApplication::platformName()=="xcb","Supply startup with native X11 widget platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"MarkPoint fixture unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"),report=files.filePath("plot results Δ.csv"),saved=files.filePath("plot commands Ω.script");
      auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),bytes);};
      MainWindow window;require(window.initialize(startup),"MarkPoint initialization failed");window.resize(window.screen()->availableGeometry().size()-QSize(80,80));window.show();events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *area=window.findChild<QMdiArea *>("workspace");auto *tree=window.findChild<QTreeWidget *>("Mission");require(source&&messages&&area&&tree,"MarkPoint workspace missing");
      const double radius=7000,mu=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant(),speed=std::sqrt(mu/radius),omega=std::sqrt(mu/(radius*radius*radius));
      QString initial="% MarkPoint witness Δ\nCreate Variable Keep;\nKeep = 41;\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(speed,'g',17)+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.SRP = Off;\nFM.Drag = None;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nP.Accuracy = 1e-12;\n";
      for(const auto &name:QStringList{"Trace","Mirror","Idle"}) initial+="Create XYPlot "+name+";\n"+name+".XVariable = Sat.ElapsedSecs;\n"+name+".YVariables = {Sat.ElapsedSecs};\n"+name+".ShowPlot = true;\n";
      initial+="Create OrbitView Orb;\nOrb.Add = {Sat};\nOrb.ShowPlot = false;\nCreate GroundTrack Map;\nMap.Add = {Sat};\nMap.ShowPlot = false;\n";
      const QString fields="Sat.ElapsedSecs Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ Keep";
      initial+="Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 30};\n";
      initial+="Report 'Before mark' Values "+fields+";\nMarkPoint 'Flag' Idle; % retain flag\nReport 'After mark' Values "+fields+";\nMarkPoint 'Repeat' Mirror Trace;\nReport 'After repeat' Values "+fields+";\nPropagate P(Sat) {Sat.ElapsedSecs = 30};\nReport 'Before second' Values "+fields+";\nMarkPoint 'Second' Mirror;\nReport 'After second' Values "+fields+";\nPropagate P(Sat) {Sat.ElapsedSecs = 30};\nReport 'Final' Values "+fields+";\n";
      initial+="% Preserve tail Ω\n";source->setPlainText(initial);require(window.buildScript(),"MarkPoint initial build failed");source->document()->setModified(false);save("initial.script",initial.toUtf8());
      const QStringList targets={"Mirror","Trace"};QString wanted=initial;QJsonArray rejected,edits;
      auto configured=[&]{QJsonArray state;auto *sat=Moderator::Instance()->GetConfiguredObject("Sat");require(sat,"MarkPoint configured spacecraft missing");for(const char *axis:{"X","Y","Z","VX","VY","VZ"})state.append(sat->GetRealParameter(axis));state.append(Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value"));return state;};const auto base=configured();
      auto open=[&](const QString &label)->QMdiSubWindow* {
         for(auto *tabs:window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);
         auto snap=window.missionSnapshot();int index=-1;for(int i=0;i<snap.nodes.size();++i)if(missionCommandLabel(snap.nodes[i].statement).name==label)index=i;require(index>=0,"MarkPoint named node absent");QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==index)item=*it;require(item,"MarkPoint tree item absent");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();acceptanceView(window);tree->itemDoubleClicked(item,0);events();auto *child=area->activeSubWindow();require(child&&dynamic_cast<CommandEditor *>(child->widget()),"MarkPoint editor missing");child->showNormal();child->setGeometry(12,12,std::min(960,area->viewport()->width()-24),std::min(680,area->viewport()->height()-24));events();acceptanceView(*child);return child;
      };
      for(const auto &label:QStringList{"Flag"}) {
         QPointer<QMdiSubWindow> child=open(label);auto *panel=dynamic_cast<CommandEditor *>(child->widget());const auto before=wanted;const int undo=source->document()->availableUndoSteps();const bool modified=source->document()->isModified();
         auto pristine=[&]{return source->toPlainText()==before&&source->document()->availableUndoSteps()==undo&&source->document()->isModified()==modified&&window.missionSnapshot().sourceScript==before&&configured()==base;};
         auto field=[&]{auto *f=panel->findChild<QLineEdit *>("commandField_Subscribers");require(f&&f->isVisible()&&f->isEnabled(),"MarkPoint typed subscriber field unreachable");return f;};
         auto click=[&](QDialogButtonBox::StandardButton button){auto *box=panel->findChild<QDialogButtonBox *>();require(box&&box->button(button)&&box->button(button)->isEnabled(),"MarkPoint action missing");acceptanceView(*child);QTest::mouseClick(box->button(button),Qt::LeftButton);events();};
         auto choose=[&]{std::exception_ptr error;bool visited=false;QTimer::singleShot(0,panel,[&]{auto *dialog=panel->findChild<QDialog *>("toggleSubscriberDialog");try{require(dialog,"MarkPoint checklist missing");auto *list=dialog->findChild<QListWidget *>("toggleSubscriberList");require(list,"MarkPoint checklist entries missing");QStringList available;for(int i=0;i<list->count();++i){auto *item=list->item(i);available<<item->text();item->setCheckState(targets.contains(item->text())?Qt::Checked:Qt::Unchecked);}require(!available.contains("Values")&&available.contains("Trace")&&available.contains("Mirror"),"MarkPoint picker includes nonplot or loses XY choices");require(!available.contains("Orb")&&!available.contains("Map")&&available.contains("Idle")&&available.size()==3,"MarkPoint picker must contain only all three XY plots");acceptanceView(*dialog);visited=true;QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok),Qt::LeftButton);}catch(...){error=std::current_exception();if(dialog)dialog->reject();}});auto *button=panel->findChild<QPushButton *>("commandChoose_Subscribers");require(button&&button->isVisible()&&button->isEnabled(),"MarkPoint checklist action unreachable");QTest::mouseClick(button,Qt::LeftButton);events();if(error)std::rethrow_exception(error);require(visited&&field()->text().split(' ',Qt::SkipEmptyParts)==targets,"MarkPoint selected subscriber order differs");};
         choose();require(panel->hasChanges()&&pristine(),"MarkPoint pending picker changed mission/model/history");click(QDialogButtonBox::Cancel);require(!child&&pristine(),"MarkPoint Cancel changed mission/model/history");child=open(label);panel=dynamic_cast<CommandEditor *>(child->widget());
         for(const auto &invalid:QStringList{"AbsentNASA","Values","Orb","Map",""})for(auto action:{QDialogButtonBox::Apply,QDialogButtonBox::Ok}){field()->setText(invalid);events();QPointer<CommandEditor> owner=panel;click(action);auto *status=owner?owner->findChild<QLabel *>("commandStatus"):nullptr;save("rejection-diagnostic.json",QJsonDocument(QJsonObject{{"source",source->toPlainText()},{"messages",messages->toPlainText()}}).toJson());require(owner&&child&&owner==panel&&panel->hasChanges()&&pristine()&&field()->text()==invalid&&status&&!status->text().isEmpty(),"MarkPoint invalid Apply/OK lost atomic source/model/history/draft/owner");rejected.append(QJsonObject{{"command",label},{"input",invalid},{"action",action==QDialogButtonBox::Apply?"Apply":"OK"},{"diagnostic",status->text()}});}
         choose();const QString command="MarkPoint";const auto oldText=command+" '"+label+"' Idle;",newText=command+" '"+label+"' "+targets.join(' ')+";";require(wanted.count(oldText)==1,"MarkPoint exact replacement target ambiguous");wanted.replace(oldText,newText);click(QDialogButtonBox::Apply);panel=dynamic_cast<CommandEditor *>(child->widget());save("apply-diagnostic.json",QJsonDocument(QJsonObject{{"source",source->toPlainText()},{"messages",messages->toPlainText()}}).toJson());require(panel&&!panel->hasChanges()&&source->toPlainText()==wanted,"MarkPoint Apply did not commit exact source");const int afterUndo=source->document()->availableUndoSteps();click(QDialogButtonBox::Apply);require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==afterUndo,"MarkPoint repeated Apply changed history");click(QDialogButtonBox::Close);require(!child,"MarkPoint clean close retained owner");source->undo();require(source->toPlainText()==before&&window.buildScript()&&configured()==base,"MarkPoint Undo failed exact restoration");source->redo();require(source->toPlainText()==wanted&&window.buildScript(),"MarkPoint Redo failed");edits.append(QJsonObject{{"label",label},{"subscribers",targets.join(' ')}});
      }
      require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==wanted,"MarkPoint Unicode save/reopen failed");save("mission.script",wanted.toUtf8());save("rejections.json",QJsonDocument(rejected).toJson());
      for(const auto &label:QStringList{"Flag"}){auto *child=open(label);auto *panel=dynamic_cast<CommandEditor *>(child->widget());require(panel->findChild<QLineEdit *>("commandField_Subscribers")->text()==targets.join(' '),"MarkPoint reopened typed subscribers differ");auto *box=panel->findChild<QDialogButtonBox *>();QTest::mouseClick(box->button(QDialogButtonBox::Ok),Qt::LeftButton);events();require(!area->subWindowList().contains(child),"MarkPoint clean OK retained owner");}
      const int runUndo=source->document()->availableUndoSteps();
      auto snap=window.missionSnapshot();int breakCount=0;
      for(int i=0;i<snap.nodes.size();++i)if(snap.nodes[i].type=="Report"){
         require(window.setBreakpoint(i,true),"MarkPoint breakpoint unavailable");++breakCount;
      }
      require(breakCount==6,"MarkPoint phase boundaries missing");
      Debugger *debug=nullptr;
      for(auto *object:window.children())if(auto *d=dynamic_cast<Debugger *>(object))debug=d;
      require(debug,"MarkPoint debugger missing");
      const QStringList names={"Mirror","Trace","Idle"};
      const auto available=area->viewport()->size();
      const int wide=std::min(790,available.width()-8),tall=std::min(590,available.height()-8);
      const int narrow=std::min(590,wide-150),shorter=std::min(390,tall-150);
      const QList<QSize> sizes={QSize(narrow,shorter),QSize(wide,shorter),QSize(narrow,tall)};
      save("window-shapes.json",QJsonDocument(QJsonObject{{"available",QJsonArray{available.width(),available.height()}},{"shapes",QJsonArray{QJsonArray{narrow,shorter},QJsonArray{wide,shorter},QJsonArray{narrow,tall}}}}).toJson());
      QMap<QString,QList<QImage>> before,after,beforeSecond;
      QMap<QString,QJsonArray> oldSamples,firstSamples;
      QJsonArray phases,pixelWitnesses;
      int calls=0;QTimer timer,timeout;timer.setInterval(10);timeout.setSingleShot(true);timeout.setInterval(60000);
      std::exception_ptr failure;
      auto viewFor=[&](const QString &name){
         require(window.plotReceiver()->show(name),"MarkPoint live viewer missing");events();
         auto *child=area->activeSubWindow();auto *view=child?dynamic_cast<PlotWidget *>(child->widget()):nullptr;
         require(view,"MarkPoint real plot widget missing");child->showNormal();area->setActiveSubWindow(child);child->raise();
         return qMakePair(child,view);
      };
      // Fixed axes and a red diagonal give an independent endpoint-location oracle.
      // Set these through the real style controls before any mark is executed.
      auto style=[&](const QString &name){
         auto pair=viewFor(name);auto *view=pair.second;std::exception_ptr error;bool done=false;
         QTimer::singleShot(0,view,[&]{auto *dialog=view->findChild<QDialog *>("plotStyleDialog");try{
            require(dialog,"MarkPoint style dialog missing");auto *tabs=dialog->findChild<QTabWidget *>("plotStyleTabs");require(tabs,"MarkPoint style tabs missing");
            dialog->findChild<QCheckBox *>("plotGrid")->setChecked(false);
            dialog->findChild<QCheckBox *>("plotLegend")->setChecked(false);
            for(int index=1;index<=2;++index){
               tabs->setCurrentIndex(index);events();const QString prefix=index==1?"plotXAxis":"plotYAxis";
               dialog->findChild<QCheckBox *>(prefix+"MinimumEnabled")->setChecked(true);
               dialog->findChild<QCheckBox *>(prefix+"MaximumEnabled")->setChecked(true);
               dialog->findChild<QLineEdit *>(prefix+"Minimum")->setText("0");
               dialog->findChild<QLineEdit *>(prefix+"Maximum")->setText("120");acceptanceView(*dialog);
            }
            tabs->setCurrentIndex(3);events();auto *page=dialog->findChild<QWidget *>("curveStyle_0");require(page&&page->isVisible(),"MarkPoint style curve page missing");
            page->findChild<QCheckBox *>("Lines")->setChecked(true);page->findChild<QCheckBox *>("Markers")->setChecked(false);
            page->findChild<QSpinBox *>("Line width")->setValue(3);page->findChild<QSpinBox *>("Marker size")->setValue(8);acceptanceView(*dialog);
            std::exception_ptr colorError;bool picked=false;
            QTimer::singleShot(0,dialog,[&]{auto *picker=qobject_cast<QColorDialog *>(QApplication::activeModalWidget());try{
               require(picker,"MarkPoint color picker missing");picker->setCurrentColor(Qt::red);acceptanceView(*picker);picked=true;picker->accept();
            }catch(...){colorError=std::current_exception();if(picker)picker->reject();}});
            QTest::mouseClick(page->findChild<QPushButton *>("Curve color"),Qt::LeftButton);
            if(colorError)std::rethrow_exception(colorError);require(picked,"MarkPoint color selection not visited");
            QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok),Qt::LeftButton);done=true;
         }catch(...){error=std::current_exception();if(dialog)dialog->reject();}});
         view->findChild<QAction *>("plotStyleAction")->trigger();events();if(error)std::rethrow_exception(error);
         auto model=window.plotReceiver()->model(name);require(done&&model&&curve(*model).color==QColor(Qt::red)&&!curve(*model).markers&&curve(*model).markerSize==8&&!model->grid&&!model->legend&&model->xAxis.minimum==0&&model->xAxis.maximum==120&&model->yAxis.minimum==0&&model->yAxis.maximum==120,"MarkPoint independent rendering fixture not applied");
      };
      auto capture=[&](const QString &name,int phase){
         auto pair=viewFor(name);QList<QImage> images;
         for(int i=0;i<sizes.size();++i){
            require(area->viewport()->width()>=sizes[i].width()&&area->viewport()->height()>=sizes[i].height(),"MarkPoint resize fixture exceeds normal workspace");
            pair.first->setGeometry(QRect(QPoint(0,0),sizes[i]));pair.second->refresh();events();acceptanceView(*pair.first);
            auto image=pair.second->canvas()->captureImage().convertToFormat(QImage::Format_ARGB32);
            require(!image.isNull()&&image.width()>300&&image.height()>180,"MarkPoint rendered image missing");images.append(image);
            if(!evidence.isEmpty()&&(phase<2||((phase==3||phase==4)&&name=="Mirror")))
               require(image.save(QDir(evidence).filePath(QString("phase%1-%2-size%3.png").arg(phase).arg(name).arg(i))),"MarkPoint direct visual oracle write failed");
         }
         require(images[1].width()>images[0].width()+100&&images[2].height()>images[0].height()+100,"MarkPoint three distinct resize shapes not exercised");
         return images;
      };
      auto location=[&](const QString &name,const QList<QImage> &a,const QList<QImage> &b,double fraction){
         QList<QPointF> centers;
         for(int i=0;i<a.size();++i){
            require(a[i].size()==b[i].size(),"MarkPoint image pair dimensions differ");
            QRect red,changed;int differences=0,redPixels=0;
            for(int y=0;y<a[i].height();++y)for(int x=0;x<a[i].width();++x){
               auto c=a[i].pixelColor(x,y);
               if(c.red()>220&&c.green()<35&&c.blue()<35){red=red.united(QRect(x,y,1,1));++redPixels;}
               if(a[i].pixel(x,y)!=b[i].pixel(x,y)){changed=changed.united(QRect(x,y,1,1));++differences;}
            }
            const double scale=double(a[i].width())/(sizes[i].width()-8);
            require(redPixels>30&&red.width()>35*scale&&red.height()>20*scale,"MarkPoint independent pre-mark diagonal missing");
            require(differences>8&&differences<1000*scale*scale&&changed.width()<=24*scale&&changed.height()<=24*scale,"MarkPoint changed more than one compact marker region");
            const QPointF center=QRectF(changed).center(),endpoint(red.right(),red.top());
            require(std::abs(center.x()-endpoint.x())<=5*scale&&std::abs(center.y()-endpoint.y())<=5*scale,"MarkPoint rendered highlight is not at independent latest endpoint");
            centers.append(center);pixelWitnesses.append(QJsonObject{{"plot",name},{"phase",calls},{"size",i},{"image_width",a[i].width()},{"image_height",a[i].height()},{"changed_pixels",differences},{"center",QJsonArray{center.x(),center.y()}},{"endpoint",QJsonArray{endpoint.x(),endpoint.y()}},{"scale",scale},{"fraction",fraction}});
         }
         require(std::abs((centers[1].x()-centers[0].x())-fraction*(a[1].width()-a[0].width()))<5,"MarkPoint horizontal resize position differs from fixed-axis fraction");
         require(std::abs((centers[2].y()-centers[0].y())-(1-fraction)*(a[2].height()-a[0].height()))<5,"MarkPoint vertical resize position differs from fixed-axis fraction");
      };
      QObject::connect(&timer,&QTimer::timeout,&window,[&]{if(!debug->isWaiting())return;timer.stop();try{
         const double time=calls<3?30:calls<5?60:90;QJsonObject output;
         for(const auto &name:names){
            auto model=window.plotReceiver()->model(name);require(bool(model),"MarkPoint live model missing");
            if(calls==0)style(name);
            const auto raw=samples(*model);require(raw.size()>=4,"MarkPoint accepted samples missing");
            QJsonArray highlighted;double previous=-1;int pointIndex=0;
            for(const auto &p:curve(*model).points){
               require(std::isfinite(p.x)&&std::abs(p.x-p.y)<1e-10&&p.x>=previous-1e-5,"MarkPoint numerical diagonal is invalid or reversed");previous=p.x;
               if(p.highlighted){highlighted.append(QJsonObject{{"index",pointIndex},{"elapsed",p.x}});require(std::abs(p.x-30)<1e-4||(name=="Mirror"&&calls>=4&&std::abs(p.x-60)<1e-4),"MarkPoint highlighted wrong data point");}++pointIndex;
            }
            require(std::abs(previous-time)<1e-4,"MarkPoint final sample time differs");
            const int count=name=="Idle"||calls==0?0:(name=="Mirror"&&calls>=4?2:1);
            require(highlighted.size()==count,"MarkPoint marker count or unselected isolation differs");
            const auto images=capture(name,calls);
            if(calls==0){before[name]=images;oldSamples[name]=raw;}
            if(calls==1){
               auto expected=oldSamples[name];if(name!="Idle"){auto last=expected.last().toArray();last[6]=true;expected[expected.size()-1]=last;location(name,before[name],images,.25);}
               else for(int i=0;i<images.size();++i)require(images[i]==before[name][i],"MarkPoint changed unselected plot pixels");
               require(raw==expected,"MarkPoint changed numerical samples, connectivity or a noncurrent highlight");after[name]=images;firstSamples[name]=raw;
            }
            if(calls==2){require(raw==firstSamples[name],"Repeated MarkPoint changed sample or marker state");for(int i=0;i<images.size();++i)require(images[i]==after[name][i],"Repeated MarkPoint changed rendered pixels");}
            if(calls==3){beforeSecond[name]=images;oldSamples[name]=raw;}
            if(calls==4){
               auto expected=oldSamples[name];if(name=="Mirror"){auto last=expected.last().toArray();last[6]=true;expected[expected.size()-1]=last;location(name,beforeSecond[name],images,.5);}
               else for(int i=0;i<images.size();++i)require(images[i]==beforeSecond[name][i],"Second MarkPoint changed an unselected plot");
               require(raw==expected,"Second MarkPoint changed numerical samples or existing marks");
            }
            require(samples(*model)==raw,"MarkPoint resizing/rendering changed retained data");
            output[name]=QJsonObject{{"points",raw},{"highlighted",highlighted}};
         }
         phases.append(QJsonObject{{"phase",calls},{"elapsed",time},{"outputs",output}});save("phases.json",QJsonDocument(phases).toJson());save("pixel-witnesses.json",QJsonDocument(pixelWitnesses).toJson());
         ++calls;debug->resume();timer.start();
      }catch(...){failure=std::current_exception();window.stopMission();}});
      QObject::connect(&timeout,&QTimer::timeout,&window,[&]{failure=std::make_exception_ptr(std::runtime_error("MarkPoint observer timed out"));window.stopMission();});
      timer.start();timeout.start();messages->clear();const auto result=window.runMission();timer.stop();timeout.stop();save("messages.txt",messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);
      require(result==MainWindow::RunResult::Completed&&calls==6,"MarkPoint mission or phase count failed");
      const auto raw=read(report);save("values.csv",raw);const auto lines=QString::fromUtf8(raw).trimmed().split('\n');require(lines.size()==6,"MarkPoint report row count differs");QJsonArray rows;
      for(int i=0;i<6;++i){const auto columns=lines[i].split(',');require(columns.size()==8,"MarkPoint report column count differs");const double t=i<3?30:i<5?60:90,angle=omega*t;const QList<double> expected={t,radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0,41};QJsonArray row;
         for(int j=0;j<8;++j){bool ok=false;double value=columns[j].trimmed().toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[j])<(j==0?1e-4:j<4?1e-3:1e-6),"MarkPoint changed independent circular-state/time/sentinel result");row.append(value);}rows.append(row);
      }
      require(source->toPlainText()==wanted&&source->document()->availableUndoSteps()==runUndo&&!window.isRunning()&&window.findChild<QAction *>("runMission")->isEnabled(),"MarkPoint execution changed source/history or failed to return GUI control");
      const QJsonObject proof{{"checkpoint","markpoint-result"},{"outcome","passed"},{"phase_count",calls},{"targets",QJsonArray::fromStringList(targets)},{"mu",mu},{"radius",radius},{"rows",rows},{"rejections",rejected},{"pixel_witnesses",pixelWitnesses},{"facets",QJsonArray{"actual-mission-tree-editor-and-xy-only-picker","pending-cancel-invalid-apply-ok-atomicity","exact-apply-idempotence-undo-redo-unicode-reopen","live-single-multi-repeat-point-selection-and-unselected-isolation","independent-rendered-latest-point-location-at-three-window-sizes","independent-circular-state-time-and-sentinel-results"}},{"scope","MarkPoint on one-curve XY plots at two propagation boundaries: multi-target, repeat/idempotent, later single-target mark, preservation and unselected isolation. Fixed-axis diagonal rendered marker location checked at three normal window shapes. Qt widget events, private native X11 exposure and independent numerical results; not native OS input, every curve/marker/style/solver/scientific domain, host GPU, platforms, stress or full current-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
