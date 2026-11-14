#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "Debugger.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "OrbitView.hpp"
#include "GroundTrack.hpp"
#include <QDialog>
#include <QDialogButtonBox>
#include <QDynamicPropertyChangeEvent>
#include <QSpinBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <set>
using namespace formaccept;

class PublicationObserver final : public Subscriber {
public:
   std::function<void(const Real *,Integer)> observe;
   std::function<void(bool)> flushed;
   PublicationObserver():Subscriber("CadenceObserver","CadenceObserver"){}
   ~PublicationObserver()override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone()const override{return new PublicationObserver(*this);}
   bool ReceiveData(const Real *data,Integer count=0)override{if(data&&count>0&&observe)observe(data,count);return true;}
   bool FlushData(bool end=true)override{if(flushed)flushed(end);return Subscriber::FlushData(end);}
   bool SetEndOfRun()override{if(flushed)flushed(true);return Subscriber::SetEndOfRun();}
protected:
   bool Distribute(Integer)override{return true;}
   bool Distribute(const Real *,Integer)override{return true;}
};
class RefreshObserver final : public QObject {
public:
   std::function<void(PlotWidget *)> observe;
   bool eventFilter(QObject *object,QEvent *event)override{
      if(event->type()==QEvent::DynamicPropertyChange&&static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName()=="gmatRefreshSerial")
         if(auto *plot=dynamic_cast<PlotWidget *>(object))if(observe)observe(plot);
      return false;
   }
};
static void reveal(QWidget *control)
{
   require(control,"Cadence input absent");
   for(auto *p=control;p;p=p->parentWidget()){
      for(auto *tabs:control->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(control);
   }
   events();require(control->isVisible()&&control->isEnabled(),"Cadence input inaccessible");acceptanceView(*control);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaPlotCadence");
   try{
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QStringList names{"Orb","Map"};const int frequencies[]={3,4};
      const double speed=std::sqrt(mu/7000.);const QString mission="BeginMissionSequence;\nReport 'Arm observer' Values Keep;\nPropagate P(Sat) {Sat.ElapsedSecs = 600};\nReport 'Final values' Values Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Keep;\n% preserve cadence Ω\n";
      const QString script="Create Spacecraft Sat;\nSat.NAIFId = -10009999;\nSat.NAIFIdReferenceFrame = -9009999;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+number(speed)+";\nSat.VZ = 0;\nCreate Variable Keep;\nKeep = 41;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\nP.Accuracy = 0.000000000001;\nCreate OrbitView Orb;\nOrb.Add = {Sat};\nOrb.CoordinateSystem = EarthMJ2000Eq;\nOrb.DataCollectFrequency = 1;\nOrb.UpdatePlotFrequency = 1;\nCreate GroundTrackPlot Map;\nMap.Add = {Sat};\nMap.DataCollectFrequency = 1;\nMap.UpdatePlotFrequency = 1;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\n"+mission;
      rig.source->setPlainText(script);require(rig.window.buildScript(),"Cadence fixture build failed");rig.source->document()->setModified(false);
      for(int i=0;i<2;++i){
         const auto before=rig.source->toPlainText();auto *native=Moderator::Instance()->GetConfiguredObject(names[i].toStdString());require(native,"Cadence native plot absent");const auto original=native->GetGeneratingString(Gmat::SCRIPTING);
         auto matches=rig.resources->findItems(names[i],Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Cadence resource item ambiguous");rig.resources->itemDoubleClicked(matches.first(),0);events();auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"Cadence resource panel absent");
         const QString prefix=i==0?"orbit_":"ground_";
         for(auto pair:QList<QPair<QString,int>>{{"DataCollectFrequency",2},{"UpdatePlotFrequency",frequencies[i]}}){
            auto *spin=panel->findChild<QSpinBox *>(prefix+pair.first);reveal(spin);spin->setFocus();QTest::keyClick(spin,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(spin,QString::number(pair.second));QTest::keyClick(spin,Qt::Key_Tab);events();require(spin->value()==pair.second,"Cadence typed value rejected");
         }
         require(panel->hasChanges()&&rig.source->toPlainText()==before&&native->GetGeneratingString(Gmat::SCRIPTING)==original,"Cadence draft changed applied graph");auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");reveal(buttons->button(QDialogButtonBox::Apply));QTest::mouseClick(buttons->button(QDialogButtonBox::Apply),Qt::LeftButton);events();require(!panel->hasChanges(),"Cadence Apply failed");const auto applied=rig.source->toPlainText();require(applied.endsWith(mission),"Cadence Apply changed mission");rig.closePanels();
         rig.source->undo();require(rig.source->toPlainText()==before&&rig.window.buildScript(),"Cadence exact Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Cadence exact Redo failed");
      }
      const auto applied=rig.source->toPlainText();const auto path=rig.files.filePath("cadence Ω.script");require(rig.window.saveScriptTo(path)&&rig.window.loadScript(path)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Cadence Unicode roundtrip failed");
      for(int i=0;i<2;++i){auto *native=Moderator::Instance()->GetConfiguredObject(names[i].toStdString());require(native->GetIntegerParameter("DataCollectFrequency")==2&&native->GetIntegerParameter("UpdatePlotFrequency")==frequencies[i],"Reopened native cadence configuration differs");}
      Debugger *debug=nullptr;for(auto *o:rig.window.children())if(auto *d=dynamic_cast<Debugger *>(o))debug=d;require(debug,"Cadence debugger absent");
      const auto snapshot=rig.window.missionSnapshot();int arm=-1;for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Arm observer"))arm=i;require(arm>=0&&rig.window.setBreakpoint(arm,true),"Cadence arm boundary absent");
      QMap<PlotWidget *,int> observed;QVector<int> refreshFrames[2];QJsonArray refreshes,samples,ignored,pending,diagnostics;std::exception_ptr failure;int rawCount=0;PublicationObserver publication;RefreshObserver refresh;bool subscribed=false,armed=false;
      refresh.observe=[&](PlotWidget *plot){try{
         if(!armed||!observed.contains(plot))return;const int index=observed[plot];auto model=rig.window.plotReceiver()->model(names[index]);require(bool(model),"Cadence refreshed model absent");const int frame=model->frame;
         require(plot->property("gmatLastRefreshFrame").toULongLong()==model->frame,"Cadence observer frame differs from completed widget refresh");
         QJsonObject row{{"plot",names[index]},{"frame",frame},{"publications_already_observed",rawCount},{"end_of_run",model->endOfRun}};
         if(frame==0||model->endOfRun){ignored.append(row);return;}
         pending.append(row);
      }catch(...){failure=std::current_exception();}};
      publication.flushed=[&](bool end){try{
         for(auto value:pending){auto row=value.toObject();const auto name=row["plot"].toString();auto model=rig.window.plotReceiver()->model(name);require(model&&row["frame"].toInt()==int(model->frame),"Native flush refresh refers to a stale frame");row["classification"]="native-end-of-data-block";row["end_of_block"]=end;ignored.append(row);}pending={};
      }catch(...){if(!failure)failure=std::current_exception();}};
      publication.observe=[&](const Real *data,Integer count){if(failure){events();return;}try{
         require(armed&&count>=7&&std::isfinite(data[0]),"Cadence raw publisher payload invalid");++rawCount;const double seconds=(data[0]-21545)*86400;
         require(std::abs(seconds-std::min(600.,10.*(rawCount-1)))<.0001,"Native publication epochs differ from the literal ten-second integration grid");
         // Source-reviewed active backends have different initial phases:
         // OrbitView retains publication1 and each even ordinal; GroundTrack
         // retains publication1 and each second publication after it. Count
         // those independent raw-stream ordinals, never the GUI filter output.
         const int collectedByPlot[]={1+rawCount/2,(rawCount+1)/2};const auto truth=circular(orbits[0],seconds);
         for(auto value:pending){auto row=value.toObject();const int index=names.indexOf(row["plot"].toString());require(index>=0,"Unknown refreshed plot");row["classification"]="regular-publication";refreshFrames[index].append(row["frame"].toInt());refreshes.append(row);}pending={};
         QJsonObject diagnostic{{"publication",rawCount},{"count",count},{"epoch",data[0]},{"expected_collected",QJsonArray{collectedByPlot[0],collectedByPlot[1]}}};for(const auto &name:names){auto model=rig.window.plotReceiver()->model(name);diagnostic[name]=model?int(model->frame):-1;}diagnostics.append(diagnostic);rig.save("cadence-collection-diagnostics.json",QJsonDocument(diagnostics).toJson());
         for(int n=0;n<6;++n)require(std::isfinite(data[n+1])&&std::abs(data[n+1]-truth[n])<(n<3?.00003:.00000004),"Cadence raw publisher state differs from independent circular oracle");
         QJsonObject row{{"publication",rawCount},{"epoch",data[0]},{"elapsed",seconds},{"independent_collected",QJsonArray{collectedByPlot[0],collectedByPlot[1]}}};
         for(int i=0;i<2;++i){const int collected=collectedByPlot[i];auto model=rig.window.plotReceiver()->model(names[i]);require(model&&!model->endOfRun&&model->frame==quint64(collected),"Cadence model collection differs from independent publisher counter");
            QVector<int> expected;for(int frame=frequencies[i];frame<=collected;frame+=frequencies[i])expected.append(frame);
            require(refreshFrames[i]==expected,"Intermediate widget refreshes violate every-N-collected cadence");row[names[i]]=QJsonObject{{"collected",int(model->frame)},{"refresh_count",refreshFrames[i].size()}};
         }
         samples.append(row);rig.save("cadence-samples.json",QJsonDocument(samples).toJson());rig.save("cadence-refreshes.json",QJsonDocument(refreshes).toJson());events();
      }catch(...){failure=std::current_exception();QTimer::singleShot(0,&rig.window,[&]{rig.window.stopMission();});}};
      auto previous=debug->paused;struct RestorePause{Debugger *debug;std::function<void(const QString &)> previous;~RestorePause(){debug->paused=previous;}}restore{debug,previous};debug->paused=[&](const QString &kind){if(previous)previous(kind);try{
         require(!armed&&kind=="Report","Cadence boundary identity differs");auto *dialog=rig.window.findChild<QDialog *>("debuggerDialog");require(dialog&&dialog->property("commandIndex").toInt()==arm,"Cadence arm command differs");
         for(int i=0;i<2;++i){auto model=rig.window.plotReceiver()->model(names[i]);require(model&&model->frame==0&&!model->endOfRun,"Cadence initial model is not empty");PlotWidget *plot=nullptr;for(auto *child:rig.area->subWindowList())if(child->property("plotName").toString()==names[i])plot=dynamic_cast<PlotWidget *>(child->widget());require(plot&&plot->canvas(),"Cadence actual viewer absent");require(!plot->property("gmatObserveRefresh").toBool()&&!plot->property("gmatRefreshSerial").isValid(),"Cadence observation unexpectedly enabled by default");plot->refresh();require(!plot->property("gmatRefreshSerial").isValid()&&model->frame==0,"Default refresh observation changed data/properties");observed[plot]=i;plot->installEventFilter(&refresh);plot->setProperty("gmatObserveRefresh",true);}
         bool orbitBackend=false,groundBackend=false;for(auto *subscriber:Publisher::Instance()->GetSubscriberList()){
            if(subscriber->GetName()=="Orb")orbitBackend=dynamic_cast<OrbitView *>(subscriber)!=nullptr;
            if(subscriber->GetName()=="Map")groundBackend=dynamic_cast<GroundTrack *>(subscriber)!=nullptr;
         }require(orbitBackend&&groundBackend,"Cadence active native backend differs from reviewed factory/collection contracts");
         require(Publisher::Instance()->Subscribe(&publication),"Cadence independent subscriber attach failed");subscribed=true;armed=true;acceptanceView(*dialog);
      }catch(...){failure=std::current_exception();QTimer::singleShot(0,&rig.window,[&]{rig.window.stopMission();});}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
      const int undo=rig.source->document()->availableUndoSteps();acceptanceView(rig.window);const auto status=rig.window.runMission();debug->paused=previous;if(subscribed)Publisher::Instance()->Unsubscribe(&publication);armed=false;events();rig.save("cadence-refreshes.json",QJsonDocument(refreshes).toJson());rig.save("cadence-ignored-refreshes.json",QJsonDocument(ignored).toJson());rig.save("cadence-messages.txt",rig.messages->toPlainText().toUtf8());if(failure)std::rethrow_exception(failure);
      require(status==MainWindow::RunResult::Completed&&rawCount>=50&&samples.size()==rawCount&&pending.isEmpty(),"Cadence native execution/publication matrix incomplete");
      for(int i=0;i<2;++i){auto model=rig.window.plotReceiver()->model(names[i]);require(model&&model->endOfRun&&model->frame==quint64(i==0?1+rawCount/2:(rawCount+1)/2),"Cadence completion lost samples");require(refreshFrames[i].size()>=5,"Cadence has too few intermediate cycles");std::set<int> residues;for(const auto &sample:samples)residues.insert(sample.toObject()[names[i]].toObject()["collected"].toInt()%frequencies[i]);require(residues.size()==size_t(frequencies[i]),"Cadence interval boundary classes missing");}
      const auto bytes=read(rig.report);rig.save("cadence.csv",bytes);const auto rows=QString::fromUtf8(bytes).trimmed().split('\n');require(rows.size()==2&&rows[0].toDouble()==41,"Cadence arm report/sentinel differs");const auto columns=rows[1].split(',');require(columns.size()==8,"Cadence final report schema differs");const auto truth=circular(orbits[0],600);for(int i=0;i<8;++i){bool ok=false;const double value=columns[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-(i==0?600:i==7?41:truth[i-1]))<(i==0?.0001:i<4?.00003:.00000004),"Cadence final report differs from independent circular/time/sentinel oracle");}
      require(rig.source->toPlainText()==applied&&rig.source->document()->availableUndoSteps()==undo&&read(path)==applied.toUtf8(),"Cadence Run changed source/history/file");rig.save("cadence.script",applied.toUtf8());
      const QJsonObject proof{{"checkpoint","native-intermediate-redraw-cadence"},{"outcome","passed"},{"plot_count",2},{"collect_frequency",2},{"collector_contracts",QJsonArray{"OrbitView: initial and even publication ordinals","GroundTrack: initial and every second later publication"}},{"update_frequencies",QJsonArray{3,4}},{"publication_count",rawCount},{"samples",samples},{"refreshes",refreshes},{"ignored_initial_or_final_refreshes",ignored},{"facets",QJsonArray{"actual-typed-plot-frequency-pending-apply-undo-redo-unicode-reopen","independent-native-publication-counter-and-circular-state","completed-production-widget-refresh-observation","every-intermediate-N-collected-threshold-and-no-early-refresh","initial-block-flush-and-final-forced-refreshes-not-cadence-credit","raw-report-time-sentinel-source-history-preservation"}}};rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
