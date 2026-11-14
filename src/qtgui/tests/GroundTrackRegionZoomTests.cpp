#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include <QAction>
#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QRubberBand>
#include <QTest>
#include <QWheelEvent>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
void events() { QApplication::processEvents(); }
QByteArray samples(const PlotModel &model)
{
   QByteArray result;
   QDataStream stream(&result,QIODevice::WriteOnly);
   for (const auto &curve:model.curves) for (const auto &point:curve.points)
      stream<<point.frame<<point.epoch<<point.x<<point.y<<point.z;
   return result;
}
QImage logicalImage(PlotCanvas &canvas)
{
   auto image=canvas.captureImage().scaled(canvas.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
   image.setDevicePixelRatio(1);
   return image;
}
QRectF mapArea(PlotCanvas &canvas)
{
   const auto image=logicalImage(canvas);
   int left=image.width(),right=-1,top=image.height(),bottom=-1;
   for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x)
      if (image.pixelColor(x,y)==QColor(35,110,165)) {
         left=std::min(left,x); right=std::max(right,x);
         top=std::min(top,y); bottom=std::max(bottom,y);
      }
   require(right>left && bottom>top,"Ground map was not painted");
   return QRectF(left,top,right-left+1,bottom-top+1);
}
void move(PlotCanvas &canvas,QPointF position,Qt::KeyboardModifiers modifiers=Qt::NoModifier)
{
   QMouseEvent event(QEvent::MouseMove,position,canvas.mapToGlobal(position.toPoint()),
                     Qt::NoButton,Qt::LeftButton,modifiers);
   QApplication::sendEvent(&canvas,&event); events();
}
void hover(PlotCanvas &canvas,QPointF position,QPointF expected,double tolerance)
{
   QMouseEvent event(QEvent::MouseMove,position,canvas.mapToGlobal(position.toPoint()),
                     Qt::NoButton,Qt::NoButton,Qt::NoModifier);
   QApplication::sendEvent(&canvas,&event);
   const auto match=QRegularExpression("Longitude: ([+-]?[0-9.]+) deg · Latitude: ([+-]?[0-9.]+) deg").match(canvas.toolTip());
   require(match.hasMatch(),"Zoomed map lost geographic hover coordinates");
   require(std::abs(match.captured(1).toDouble()-expected.x())<tolerance &&
           std::abs(match.captured(2).toDouble()-expected.y())<tolerance,
           "Zoomed map hover differs from the selected geographic range");
}
QPointF fraction(const QRectF &area,double x,double y)
{
   return {area.left()+x*area.width(),area.top()+y*area.height()};
}
QRectF selectedRange(const QRectF &range,const QRectF &area,QPointF start,QPointF end)
{
   const auto region=QRectF(start,end).normalized().intersected(area);
   return {range.left()+(region.left()-area.left())/area.width()*range.width(),
           range.top()+(area.bottom()-region.bottom())/area.height()*range.height(),
           region.width()/area.width()*range.width(),region.height()/area.height()*range.height()};
}
bool redNear(const QImage &image,QPointF point)
{
   for (int y=qRound(point.y())-6;y<=qRound(point.y())+6;++y)
      for (int x=qRound(point.x())-6;x<=qRound(point.x())+6;++x)
         if (image.rect().contains(x,y)) {
            const auto color=image.pixelColor(x,y);
            if (color.red()>180 && color.green()<80 && color.blue()<80) return true;
         }
   return false;
}
}

int main(int argc,char **argv)
{
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests"); application.setApplicationName("GroundTrackRegionZoom");
   TestSettings settings;
   try {
      const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!directory.isEmpty()) QDir().mkpath(directory);
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::GroundTrack);
      model->title="Ground map region zoom"; model->legend=false; model->grid=false; model->labels=false;
      model->map=QImage(360,180,QImage::Format_RGB32); model->map.fill(QColor(35,110,165));
      auto &curve=model->curves[0]; curve.color=Qt::red; curve.lines=false;
      curve.markers=true; curve.markerType=4; curve.markerSize=7;
      for (int longitude=-120;longitude<=120;longitude+=30) {
         ++model->frame; model->append(0,longitude,longitude*.5);
      }
      const auto originalSamples=samples(*model);
      PlotWidget plot(model); plot.resize(820,540); plot.show();
      require(QTest::qWaitForWindowExposed(&plot),"Map test window was not exposed");
      auto &canvas=*plot.canvas();
      QFont font=canvas.font(); font.setPixelSize(12); canvas.setFont(font);
      canvas.fit(); events();
      auto *band=canvas.findChild<QRubberBand *>("groundZoomSelection");
      require(band,"Map has no drag-to-select mechanism");
      require(canvas.toolTip().contains("Shift-drag: pan"),"Map gesture instructions are missing");
      const auto full=QRectF(-180,-90,360,180);
      auto area=mapArea(canvas);
      const auto baseline=logicalImage(canvas);
      int views=0,markers=0,boundaries=0;
      auto stable=[&] { require(samples(*model)==originalSamples,"Map interaction changed geographic samples"); };
      auto verify=[&](const QString &name,const QRectF &range) {
         const auto image=logicalImage(canvas); const auto viewport=mapArea(canvas);
         require(viewport.width()>canvas.width()*.7 && viewport.height()>canvas.height()*.6,"Selected map does not fill its pane");
         for (const auto &point:curve.points) if (range.adjusted(range.width()*.08,range.height()*.08,-range.width()*.08,-range.height()*.08).contains(QPointF(point.x,point.y))) {
            const QPointF screen(viewport.left()+(point.x-range.left())/range.width()*viewport.width(),
                                 viewport.bottom()-(point.y-range.top())/range.height()*viewport.height());
            require(redNear(image,screen),"Map sample is outside its independently calculated screen position"); ++markers;
         }
         const auto position=fraction(viewport,.43,.57);
         hover(canvas,position,{range.left()+.43*range.width(),range.bottom()-.57*range.height()},2.0);
         if (!directory.isEmpty()) require(image.save(directory+'/'+name+".png"),"Map screenshot could not be saved");
         stable(); ++views;
      };
      auto select=[&](QPointF start,QPointF end) {
         QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,start.toPoint()); move(canvas,end);
         require(band->isVisible(),"Map selection band did not appear");
         require(canvas.rect().contains(band->geometry()),"Map selection escaped the canvas");
         if (!directory.isEmpty()) canvas.captureImage().save(directory+"/selection-band.png");
         QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,end.toPoint()); events();
         require(!band->isVisible(),"Map selection band remained after release"); stable();
      };
      auto fit=[&] { plot.findChild<QAction *>("plotFitAction")->trigger(); events(); stable(); };
      verify("full",full);
      const auto start=fraction(area,.2,.15),end=fraction(area,.8,.65);
      const auto selected=selectedRange(full,area,start,end);
      select(start,end); verify("selected",selected);
      const auto forward=logicalImage(canvas);
      fit(); require(logicalImage(canvas)==baseline,"Fit did not restore the full map");
      select(end,start); verify("reversed",selected);
      require(logicalImage(canvas)==forward,"Reverse drag selected a different map region");
      const auto innerStart=fraction(area,.25,.25),innerEnd=fraction(area,.75,.75);
      const auto nested=selectedRange(selected,area,innerStart,innerEnd);
      select(innerStart,innerEnd); verify("nested",nested);
      QTest::mouseDClick(&canvas,Qt::LeftButton,Qt::NoModifier,area.center().toPoint()); events();
      require(logicalImage(canvas)==baseline,"Double-click did not reset the selected map range");
      for (const auto &offset:QList<QPoint>{{0,0},{2,60},{60,2}}) {
         QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,start.toPoint()); move(canvas,start+offset);
         QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,(start+offset).toPoint()); events();
         require(logicalImage(canvas)==baseline,"Tiny selection changed the map view"); ++boundaries;
      }
      for (int cancellation=0;cancellation<4;++cancellation) {
         QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,start.toPoint()); move(canvas,end);
         if (cancellation==0) QTest::keyClick(&canvas,Qt::Key_Escape);
         if (cancellation==1) QTest::mousePress(&canvas,Qt::RightButton,Qt::NoModifier,start.toPoint());
         if (cancellation==2) canvas.refresh();
         if (cancellation==3) { QEvent event(QEvent::WindowDeactivate); QApplication::sendEvent(&canvas,&event); }
         require(!band->isVisible(),"Cancelled map selection is still visible");
         QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,end.toPoint()); events();
         require(logicalImage(canvas)==baseline,"Cancelled map selection changed the view"); ++boundaries;
      }
      const auto outside=QPoint(4,4);
      QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,outside); move(canvas,end);
      require(!band->isVisible(),"Margin press started map selection");
      QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,end.toPoint()); events();
      require(logicalImage(canvas)==baseline,"Margin drag changed the map view"); ++boundaries;
      select(start,end);
      const QPoint shift(20,12),center=area.center().toPoint();
      QTest::mousePress(&canvas,Qt::LeftButton,Qt::ShiftModifier,center); move(canvas,center+shift,Qt::ShiftModifier);
      require(!band->isVisible(),"Shift-pan started a map selection");
      QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::ShiftModifier,center+shift); events();
      const auto panned=selected.translated(-shift.x()*selected.width()/area.width(),shift.y()*selected.height()/area.height());
      verify("panned",panned);
      canvas.zoomBy(1); events();
      const double factor=1.15;
      const auto zoomed=QRectF(panned.center()-QPointF(panned.width()/factor,panned.height()/factor)/2,
                               QSizeF(panned.width()/factor,panned.height()/factor));
      verify("wheel",zoomed);
      canvas.zoomBy(-1); events(); verify("wheel-restored",panned);
      plot.resize(1020,660); events(); verify("resized",panned);
      QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.rect().center()); move(canvas,canvas.rect().center()+QPoint(80,50));
      plot.resize(920,600); events(); require(!band->isVisible(),"Resize retained stale selection coordinates");
      QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,canvas.rect().center()); events(); ++boundaries;
      fit(); area=mapArea(canvas);
      const auto edgeStart=fraction(area,.5,.5),edgeEnd=area.topRight()+QPointF(400,-400);
      const auto clipped=selectedRange(full,area,edgeStart,edgeEnd);
      select(edgeStart,edgeEnd); verify("clipped",clipped);
      fit(); canvas.zoomBy(-10); events();
      QTest::mousePress(&canvas,Qt::LeftButton,Qt::NoModifier,QPoint(70,70)); move(canvas,{100,100});
      require(!band->isVisible(),"Blank area outside the zoomed-out globe accepted a selection");
      QTest::mouseRelease(&canvas,Qt::LeftButton,Qt::NoModifier,QPoint(100,100)); events(); ++boundaries;
      fit(); stable();
      const QJsonObject result{{"status","passed"},{"views",views},{"independent_marker_probes",markers},{"boundary_checks",boundaries},
                               {"checks","forward/reverse/nested selection, map clipping, geographic hover, shift-pan, wheel, fit, resize and cancellation"}};
      if (!directory.isEmpty()) {
         QFile file(directory+"/proof.json"); require(file.open(QIODevice::WriteOnly),"Cannot save map zoom proof");
         file.write(QJsonDocument(result).toJson());
      }
      std::cout<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<'\n';
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
