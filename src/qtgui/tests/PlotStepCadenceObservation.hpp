#ifndef GMAT_PLOT_STEP_CADENCE_OBSERVATION_HPP
#define GMAT_PLOT_STEP_CADENCE_OBSERVATION_HPP
#include "PlotStepLiveObservation.hpp"
#include "PlotWidget.hpp"
#include <QDynamicPropertyChangeEvent>

class PlotStepRefresh final : public QObject {
public:
   std::function<void(PlotWidget *)> observe;
   bool eventFilter(QObject *object,QEvent *event) override {
      if(event->type()==QEvent::DynamicPropertyChange&&static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName()=="gmatRefreshSerial")
         if(auto *plot=dynamic_cast<PlotWidget *>(object))if(observe)observe(plot);
      return false;
   }
};
class PlotStepCadence {
   formaccept::Rig &rig;
   PlotStepStream stream;
   PlotStepRefresh refresh;
   QMap<PlotWidget *,int> widgets;
   QVector<int> frames[2];
   QJsonArray pending,refreshes,forced,samples;
   QStringList names{"Orb","Map"};
   int frequency[2]={3,4};
   int publications=0;
   bool armed=false;
   std::exception_ptr failure;
   void guard(const std::function<void()> &f) {
      if(failure)return;
      try {f();}catch(...) {
         failure=std::current_exception();
         QTimer::singleShot(0,&rig.window,[this]{rig.window.stopMission();});
      }
   }
   void save() {
      rig.save("CadenceRetention-observations.json",QJsonDocument(QJsonObject{
         {"samples",samples},{"regular_refreshes",refreshes},{"forced_refreshes",forced}}).toJson());
   }
   QJsonObject modelCheck(int i,int collected) {
      auto model=rig.window.plotReceiver()->model(names[i]);
      formaccept::require(model&&model->frame==quint64(collected),"Step cadence collected frame differs from literal twenty-second grid");
      const PlotCurve *curve=nullptr;
      for(const auto &c:model->curves)if(c.name=="One")curve=&c;
      const int retained=std::min(10,collected),first=collected-retained;
      QJsonArray points;
      if(curve)for(const auto &p:curve->points)points.append(QJsonObject{{"seconds",(p.epoch-21545)*86400},{"x",p.x},{"y",p.y},{"z",p.z}});
      if(points.size()!=retained)throw std::runtime_error((names[i]+" retained count differs: "+QString::number(points.size())+" vs "+QString::number(retained)).toStdString());
      for(int j=0;j<retained;++j) {
         const auto &p=curve->points[j];const double seconds=20.*(first+j);
         formaccept::require(std::abs((p.epoch-21545)*86400-seconds)<.0001&&!p.solver,"Step retention changed selected time/order/classification");
         formaccept::require(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z),"Step retention coordinate nonfinite");
         if(i==0) {
            const auto state=formaccept::circular(formaccept::orbits[0],seconds);
            formaccept::require(std::abs(p.x-state[0])<.00004&&std::abs(p.y-state[1])<.00004&&std::abs(p.z-state[2])<.00004,
               "Step retention position differs from circular oracle");
         }
      }
      return {{"plot",names[i]},{"frame",collected},{"retained",points}};
   }
public:
   explicit PlotStepCadence(formaccept::Rig &r):rig(r) {
      refresh.observe=[this](PlotWidget *plot){if(armed)guard([&]{
         formaccept::require(widgets.contains(plot),"Unknown step cadence widget");const int i=widgets[plot];
         auto model=rig.window.plotReceiver()->model(names[i]);
         formaccept::require(model&&plot->property("gmatLastRefreshFrame").toULongLong()==model->frame,"Step cadence refresh frame mismatch");
         QJsonObject row{{"plot",names[i]},{"frame",int(model->frame)},{"end_of_run",model->endOfRun}};
         if(model->frame==0||model->endOfRun) {row["classification"]="initial-or-end-of-run";forced.append(row);}
         else pending.append(row);
      });};
      stream.flush=[this]{if(armed)guard([&]{
         for(auto value:pending) {auto row=value.toObject();row["classification"]="native-flush";forced.append(row);}pending={};save();
      });};
      stream.sample=[this](const Real *data,Integer count){if(armed)guard([&]{
         formaccept::require(count>=7,"Step cadence raw payload incomplete");
         const double seconds=(data[0]-21545)*86400;
         const int step=int(std::llround(seconds/10.));
         formaccept::require(step>=0&&step<=60&&std::abs(seconds-10.*step)<.0001,"Step cadence raw time outside ten-second fixture grid");
         const auto truth=formaccept::circular(formaccept::orbits[0],seconds);
         for(int j=0;j<6;++j)formaccept::require(std::isfinite(data[j+1])&&std::abs(data[j+1]-truth[j])<(j<3?.00004:.00000005),"Step cadence raw state differs from independent circle");
         const int collected=1+step/2;
         for(auto value:pending) {auto row=value.toObject();const int i=names.indexOf(row["plot"].toString());formaccept::require(i>=0,"Unknown cadence plot");frames[i]<<row["frame"].toInt();row["classification"]="regular-publication";refreshes.append(row);}pending={};
         QJsonArray models;
         for(int i=0;i<2;++i) {
            models.append(modelCheck(i,collected));
            QVector<int> expected;for(int f=frequency[i];f<=collected;f+=frequency[i])expected<<f;
            formaccept::require(frames[i]==expected,"New-mode intermediate refresh cadence differs from every-N-collected grid");
         }
         samples.append(QJsonObject{{"raw_seconds",seconds},{"raw_step",step},{"expected_collected",collected},{"models",models}});
         ++publications;save();formaccept::events();
      });};
   }
   void arm() {guard([&]{
      for(int i=0;i<2;++i) {
         PlotWidget *plot=nullptr;
         for(auto *child:rig.area->subWindowList())if(child->property("plotName").toString()==names[i])plot=dynamic_cast<PlotWidget *>(child->widget());
         auto model=rig.window.plotReceiver()->model(names[i]);
         formaccept::require(plot&&plot->canvas()&&model&&model->frame==0,"Step cadence initial viewer absent/nonempty");
         widgets[plot]=i;plot->installEventFilter(&refresh);plot->setProperty("gmatObserveRefresh",true);
      }
      formaccept::require(Publisher::Instance()->Subscribe(&stream),"Step cadence observer attach failed");armed=true;
   });}
   QJsonObject finish() {
      Publisher::Instance()->Unsubscribe(&stream);armed=false;
      for(auto *plot:widgets.keys()){plot->removeEventFilter(&refresh);plot->setProperty("gmatObserveRefresh",false);}
      save();if(failure)std::rethrow_exception(failure);
      formaccept::require(publications>=61&&pending.isEmpty(),"Step cadence publication/flush matrix incomplete");
      for(int i=0;i<2;++i) {modelCheck(i,31);formaccept::require(rig.window.plotReceiver()->model(names[i])->endOfRun&&frames[i].size()>=7,"Step cadence final state/cycles incomplete");}
      // Require every raw grid position, including both sides of each retained
      // threshold. Adjacent duplicate command seeds are allowed, never credited.
      QVector<int> raw;for(const auto &v:samples){int step=v.toObject()["raw_step"].toInt();if(raw.isEmpty()||raw.last()!=step)raw<<step;}
      QVector<int> expected;for(int i=0;i<=60;++i)expected<<i;
      formaccept::require(raw==expected,"Step cadence raw grid skipped/reordered an advance");
      return {{"publications",publications},{"collected",31},{"retained",10},
         {"update_frequencies",QJsonArray{3,4}},{"regular_refreshes",refreshes.size()},{"forced_refreshes",forced.size()}};
   }
};
#endif
