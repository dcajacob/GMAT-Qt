#include "FormationAcceptance.hpp"
#include "PlotStepRedrawOracle.hpp"
#include "PlotWidget.hpp"
#include "QtPlotReceiver.hpp"
#include <QAction>
#include <QSlider>
#include <QTimer>
#include <QToolButton>
#include <QTest>
using namespace formaccept;

static void click(PlotWidget &widget,const QString &name)
{
   auto *action=widget.findChild<QAction *>(name);require(action,"Playback action missing");
   QToolButton *button=nullptr;
   for(auto *candidate:widget.findChildren<QToolButton *>())if(candidate->defaultAction()==action)button=candidate;
   require(button&&button->isVisible()&&button->isEnabled(),"Playback action inaccessible");
   acceptanceView(*button);QTest::mouseClick(button,Qt::LeftButton);events();
}

// Keep the complete geometry for identical camera fitting. Reference connections
// come from the literal retained-frame prefix, with automatic redraw disabled.
static QJsonObject pixels(PlotWidget &widget,const std::shared_ptr<const PlotModel> &model,const QString &phase)
{
   auto *actualCanvas=widget.canvas();const auto through=actualCanvas->frame();
   const auto actual=plotredraw::image(*actualCanvas);
   auto reference=std::make_shared<PlotModel>(*model);reference->redrawPoints=0;
   require(reference->curves.first().name=="One","Playback primary curve identity changed");
   auto &curve=reference->curves.first();int visible=0;
   for(const auto &point:curve.points)if(point.frame<=through)++visible;
   require(visible>0,"Playback prefix empty");
   const bool latest=through==std::numeric_limits<quint64>::max();
   const int wanted=latest?visible-1:std::min(2,visible-1);
   PlotCanvas canvas(reference,actualCanvas->parentWidget());
   canvas.setGeometry(actualCanvas->geometry());canvas.show();canvas.raise();canvas.setFrame(through);
   // setFrame clears the copied replay pose; install the recorded cursor only
   // for the interpolated path, preserving the same visible epoch/camera.
   if(model->replayCursor)canvas.setReplayCursor(*model->replayCursor);
   QJsonArray alternatives;bool equal=false;int rejected=0;
   const std::set<int> limits{0,std::max(0,wanted-1),wanted,std::min(visible-1,wanted+1),visible-1};
   for(int count:limits){
      for(auto &c:reference->curves)for(int i=0;i<int(c.points.size());++i)c.points[i].connect=i>0&&i>=visible-count;
      const auto predicted=plotredraw::image(canvas);const int difference=plotredraw::different(actual,predicted);
      alternatives.append(QJsonObject{{"segments",count},{"different_pixels",difference},{"reference_sha256",plotredraw::hash(predicted)}});
      if(count==wanted)equal=difference==0;
      else {require(difference>=5,"Wrong playback segment count not distinguishable");++rejected;}
   }
   require(equal&&rejected>=1,"Playback pixels differ from literal retained-frame prefix");
   return {{"phase",phase},{"frame",double(through)},{"visible_retained_points",visible},
      {"expected_full_segments",wanted},{"interpolated",model->replayCursor.has_value()},
      {"actual_sha256",plotredraw::hash(actual)},{"alternatives",alternatives}};
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("PlotPlaybackRedraw");
   try{
      require(argc==3,"Supply startup and OrbitView/GroundTrack case");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString kind=argv[2];require(kind=="OrbitView"||kind=="GroundTrack","Unknown playback plot type");
      auto script=rig.script({"One"},false);
      script.replace("P.InitialStepSize = 10;","P.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;");
      const QString plot="Create "+QString(kind=="OrbitView"?"OrbitView":"GroundTrackPlot")+" View;\nView.Add = {One};\n"+
         (kind=="OrbitView"?QString("View.CoordinateSystem = EarthMJ2000Eq;\n"):QString())+
         "View.IntegrationStepsToSkip = 9;\nView.UpdatePlotFrequency = 2;\nView.MaxPlotPoints = 6;\nView.NumPointsToRedraw = 2;\n";
      script.replace("BeginMissionSequence;",plot+"BeginMissionSequence;");script.replace("One.ElapsedSecs = 60","One.ElapsedSecs = 1000");
      rig.source->setPlainText(script);require(rig.window.buildScript(),"Playback fixture build failed");
      require(rig.window.runMission()==MainWindow::RunResult::Completed,"Playback witness mission failed");events();
      auto *receiver=rig.window.plotReceiver();auto model=receiver->model("View");
      QJsonArray configuredCurves;if(model)for(const auto &c:model->curves)configuredCurves.append(QJsonObject{{"name",c.name},{"points",int(c.points.size())}});
      rig.save("model-settings.json",QJsonDocument(QJsonObject{{"model_exists",bool(model)},{"end_of_run",model&&model->endOfRun},{"max_points",model?model->maxPoints:-1},{"redraw_points",model?model->redrawPoints:-1},{"curves",configuredCurves}}).toJson());
      require(model&&model->endOfRun&&model->maxPoints==6&&model->redrawPoints==2,"Native playback settings missing");
      QStringList actualNames;for(const auto &c:model->curves)actualNames.append(c.name);
      require(actualNames==(kind=="OrbitView"?QStringList{"One","Earth","Sun"}:QStringList{"One"}),"Native playback spacecraft/camera-body partition differs");
      const auto originalCurves=model->curves;
      const auto before=model->curves.first().points;require(before.size()==6,"Native playback retention differs");
      QJsonArray retained;
      for(int i=0;i<6;++i){const auto &p=before[i];const double t=500.+100*i;
         require(std::abs((p.epoch-epoch)*86400-t)<1e-4,"Native playback retained epochs differ");
         if(kind=="OrbitView"){const auto expected=circular(orbits[0],t);require(std::abs(p.x-expected[0])<4e-5&&std::abs(p.y-expected[1])<4e-5&&std::abs(p.z-expected[2])<4e-5,"Playback input differs from circular orbit");}
         retained.append(QJsonObject{{"frame",double(p.frame)},{"elapsed",(p.epoch-epoch)*86400},{"x",p.x},{"y",p.y},{"z",p.z}});
      }
      require(receiver->show("View"),"Cannot open native playback viewer");PlotWidget *widget=nullptr;
      for(auto *child:rig.area->subWindowList())if(child->property("plotName")=="View"){
         child->showNormal();rig.area->setActiveSubWindow(child);widget=dynamic_cast<PlotWidget *>(child->widget());
      }
      require(widget&&widget->canvas(),"Native playback widget absent");events();acceptanceView(*widget);
      auto *slider=widget->findChild<QSlider *>("plotTimeline");auto *timer=widget->findChild<QTimer *>("plotReplayTimer");
      auto *play=widget->findChild<QAction *>("plotReplayPlay");require(slider&&timer&&play,"Playback transport missing");
      QJsonArray observations;
      for(const auto mode:{PlotReplayMode::PropagationSteps,PlotReplayMode::InterpolatedSteps}){
         widget->setSharedReplayMode(mode);click(*widget,"plotReplayStart");require(slider->value()==0,"Start did not rewind");
         click(*widget,"plotReplayPlay");require(timer->isActive()&&play->isChecked(),"Play did not start timer");
         QMetaObject::invokeMethod(timer,"timeout",Qt::DirectConnection);
         click(*widget,"plotReplayPlay");require(!timer->isActive()&&!play->isChecked()&&slider->value()>0,"Pause lost replay advance");
         for(int position:{500,700}){
            slider->setValue(position);events();auto row=pixels(*widget,model,"paused-history");
            row["mode"]=mode==PlotReplayMode::PropagationSteps?"PropagationSteps":"InterpolatedSteps";row["timeline"]=position;observations.append(row);
         }
         click(*widget,"plotReplayLatest");require(slider->value()==1000&&!timer->isActive(),"Latest did not restore completed history");
         auto row=pixels(*widget,model,"latest-complete");row["mode"]=mode==PlotReplayMode::PropagationSteps?"PropagationSteps":"InterpolatedSteps";observations.append(row);
      }
      require(rig.source->toPlainText()==script&&model->curves.first().points.size()==before.size(),"Playback changed script or retained history");
      for(int i=0;i<6;++i){const auto &a=model->curves.first().points[i],&b=before[i];require(a.frame==b.frame&&a.epoch==b.epoch&&a.x==b.x&&a.y==b.y&&a.z==b.z&&a.connect==b.connect,"Playback mutated retained samples");}
      for(auto it=originalCurves.cbegin();it!=originalCurves.cend();++it){
         const auto &current=model->curves[it.key()].points;const auto &original=it.value().points;
         require(current.size()==original.size(),"Playback changed camera-body retention");
         for(int i=0;i<int(original.size());++i){const auto &a=current[i],&b=original[i];require(a.frame==b.frame&&a.epoch==b.epoch&&a.x==b.x&&a.y==b.y&&a.z==b.z&&a.connect==b.connect,"Playback mutated camera-body history");}
      }
      rig.save("applied.script",script.toUtf8());rig.save("values.csv",read(rig.report));rig.save("retained.json",QJsonDocument(retained).toJson());
      rig.save("playback-pixels.json",QJsonDocument(observations).toJson());
      rig.save("proof.json",QJsonDocument(QJsonObject{{"outcome","passed"},{"case",kind},{"retained_points",6},{"redraw_segments",2},
         {"modes",QJsonArray{"PropagationSteps","InterpolatedSteps"}},{"pixel_observations",observations.size()},
         {"actual_transport_buttons",true},{"retained_history_unchanged",true},{"new_requirement_credit",0}}).toJson());
      return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
