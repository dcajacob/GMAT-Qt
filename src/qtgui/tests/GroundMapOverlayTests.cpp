#include "GroundMapData.hpp"
#include "GroundMapOverlays.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTest>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv); TestSettings settings;
   try {
      require(groundMapPlaces().size()==7342,"Offline place catalog is incomplete");
      require(groundMapCountries().size()==242,"Country catalog is incomplete");
      require(groundMapCounties().size()==3235,"County catalog is incomplete");
      for (const auto *dataset:{&groundMapCountries(),&groundMapCounties()})
         for (const auto &item:*dataset) {
            require(!item.fine.isEmpty() && item.coarse.elementCount()<=item.fine.elementCount(),"Boundary LOD is invalid");
            for (const auto *path:{&item.coarse,&item.fine}) for (int i=1;i<path->elementCount();++i) {
               const auto before=path->elementAt(i-1),point=path->elementAt(i);
               // Antarctica's closing edge follows the bottom of the map.
               const bool polarEdge=before.y<-89.9 && point.y<-89.9;
               require(point.isMoveTo() || polarEdge || std::abs(point.x-before.x)<=180,"Boundary crosses the whole map at the antimeridian");
            }
         }
      const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!directory.isEmpty()) QDir().mkpath(directory);
      QJsonArray observations;
      for (const auto &range:QList<QRectF>{{-180,-90,360,180},{-125,25,60,30},{-109,37,7,5},{-105.5,39,1.5,1.5}}) {
         QImage image(1000,640,QImage::Format_RGB32); image.fill(QColor(40,91,115));
         QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing);
         const QRectF viewport(20,20,960,600);
         const double sx=viewport.width()/range.width(),sy=viewport.height()/range.height();
         const QTransform transform(sx,0,0,-sy,viewport.left()-range.left()*sx,viewport.bottom()+range.top()*sy);
         const auto result=drawGroundMapOverlays(painter,{true,true,true},transform,viewport);
         require(result.countryBoundaries>0 && result.cityPoints>0 && !result.labels.isEmpty(),"Visible country/city overlay is missing");
         if (range.width()==360) require(result.countyBoundaries==0,"County details clutter the world view");
         if (range.width()<10) require(result.countyBoundaries>0,"County details are missing at local zoom");
         for (int i=0;i<result.labels.size();++i) {
            require(viewport.contains(result.labels[i].bounds),"An overlay label is clipped");
            for (int j=0;j<i;++j)
               require(!result.labels[i].bounds.intersects(result.labels[j].bounds),"Overlay labels overlap");
         }
         painter.end();
         if (!directory.isEmpty()) image.save(directory+QString("/layers-%1.png").arg(observations.size()));
         observations.append(QJsonObject{{"longitude_span",range.width()},{"countries",result.countryBoundaries},
            {"counties",result.countyBoundaries},{"cities",result.cityPoints},{"labels",result.labels.size()}});
      }
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::GroundTrack);
      model->map=QImage(360,180,QImage::Format_RGB32); model->map.fill(QColor(40,91,115));
      PlotWidget plot(model); plot.resize(900,600); plot.show();
      require(QTest::qWaitForWindowExposed(&plot),"Map layer window did not open");
      plot.canvas()->refresh(); app.processEvents();
      const auto plain=plot.canvas()->captureImage();
      for (const auto &name:QStringList{"groundCountries","groundCounties","groundCities"}) {
         auto *action=plot.findChild<QAction *>(name); require(action && action->isCheckable(),"Map layer toggle is missing"); action->trigger();
      }
      app.processEvents();
      require(model->mapOverlays.countries && model->mapOverlays.counties && model->mapOverlays.cities,"Layer toggles did not update the map");
      require(plain!=plot.canvas()->captureImage(),"Enabled overlays did not change map pixels");
      model->groundBody="Mars"; plot.canvas()->refresh(); app.processEvents();
      require(plain==plot.canvas()->captureImage(),"Earth overlays were drawn on another body");
      PlotWidget mars(model);
      require(!mars.findChild<QAction *>("groundMapOverlaysAction")->isEnabled(),"Earth layers are offered on another body");
      QJsonObject proof{{"status","passed"},{"places",7342},{"countries",242},{"counties",3235},{"views",observations}};
      if (!directory.isEmpty()) {
         QFile file(directory+"/proof.json"); require(file.open(QIODevice::WriteOnly),"Cannot save layer proof"); file.write(QJsonDocument(proof).toJson());
      }
      std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<'\n';
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
