#include "GroundTrackLayout.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QTest>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}
QRectF mapBounds(const QImage &image, const QSize &logicalSize)
{
   const QColor mapColor(35, 110, 165);
   int left=image.width(), top=image.height(), right=-1, bottom=-1;
   for (int y=0; y<image.height(); ++y)
      for (int x=0; x<image.width(); ++x)
         if (image.pixelColor(x,y)==mapColor) {
            left=std::min(left,x); right=std::max(right,x);
            top=std::min(top,y); bottom=std::max(bottom,y);
         }
   require(right>=left && bottom>=top, "Ground map was not rendered");
   const double ratio=static_cast<double>(image.width())/logicalSize.width();
   return QRectF(left/ratio,top/ratio,(right-left+1)/ratio,(bottom-top+1)/ratio);
}
void checkLabels(const QVector<GroundTrackAxisLabel> &labels, const QRectF &canvas)
{
   require(!labels.isEmpty(), "Ground axis labels are missing");
   for (int i=0; i<labels.size(); ++i) {
      require(canvas.contains(labels[i].bounds), "Ground axis label was clipped");
      for (int j=0; j<i; ++j)
         require(!labels[i].bounds.intersects(labels[j].bounds.adjusted(-2,-2,2,2)),
                 "Ground axis labels overlap");
   }
}
void hover(PlotCanvas &canvas, QPointF position, double longitude, double latitude, double tolerance)
{
   QCursor::setPos(canvas.mapToGlobal(position.toPoint()));
   QApplication::processEvents();
   QMouseEvent move(QEvent::MouseMove,position,canvas.mapToGlobal(position.toPoint()),
                    Qt::NoButton,Qt::NoButton,Qt::NoModifier);
   QApplication::sendEvent(&canvas,&move);
   const auto match=QRegularExpression("Longitude: ([+-]?[0-9.]+) deg · Latitude: ([+-]?[0-9.]+) deg").match(canvas.toolTip());
   if (!match.hasMatch()) std::cerr<<"Hover at "<<position.x()<<','<<position.y()<<" in "<<canvas.width()<<'x'<<canvas.height()<<" gave "<<canvas.toolTip().toStdString()<<std::endl;
   require(match.hasMatch(), "Ground map hover coordinates are missing");
   require(std::abs(match.captured(1).toDouble()-longitude)<=tolerance &&
           std::abs(match.captured(2).toDouble()-latitude)<=tolerance,
           "Ground map hover differs from the displayed coordinate mapping");
}
}

int main(int argc,char **argv)
{
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests");
   application.setApplicationName("GroundTrackLayout");
   TestSettings settings;
   const QString directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   QJsonArray observations;
   bool passed=true;
   try {
      for (int fontSize : {10,14}) {
         auto model=std::make_shared<PlotModel>(PlotModel::Kind::GroundTrack);
         model->title="Ground track"; model->xLabel="Longitude (deg)"; model->yLabel="Latitude (deg)";
         model->map=QImage(360,180,QImage::Format_RGB32); model->map.fill(QColor(35,110,165));
         model->grid=false; model->labels=false; model->curves[0].name="Sat";
         model->curves[0].color=Qt::red;
         for (const auto &point : {QPointF(170,-20),QPointF(-170,20),QPointF(-100,40)}) {
            ++model->frame; model->append(0,point.x(),point.y());
         }
         const auto retained=model->curves[0].points;
         QWidget host;
         host.resize(1200,800);
         PlotCanvas canvas(model,&host); QFont font=canvas.font(); font.setPointSize(fontSize); canvas.setFont(font);
         host.show(); canvas.show();
         require(QTest::qWaitForWindowExposed(&host), "Ground canvas was not exposed");
         for (const QSize size : {QSize(200,140),QSize(360,180),QSize(640,360),QSize(1100,640)}) {
            canvas.resize(size); canvas.fit(); application.processEvents(); QTest::qWait(40);
            require(canvas.size()==size,"Ground canvas did not reach its requested test size");
            const auto image=canvas.captureImage(); const auto area=mapBounds(image,canvas.size());
            std::cerr<<"Layout image="<<image.width()<<'x'<<image.height()<<" canvas="<<canvas.width()<<'x'<<canvas.height()
                     <<" area="<<area.x()<<','<<area.y()<<','<<area.width()<<','<<area.height()<<std::endl;
            const double minWidth=fontSize==14 && size.width()==200 ? .55 : .68;
            const bool fills=area.width()>=size.width()*minWidth && area.height()>=size.height()*.38;
            passed=passed && fills;
            if (!directory.isEmpty()) {
               QDir().mkpath(directory);
               require(image.save(directory+QString("/ground-%1-%2x%3.png").arg(fontSize).arg(size.width()).arg(size.height())),
                       "Ground screenshot could not be saved");
            }
            const auto horizontal=groundTrackAxisLabels(area,-180,180,canvas.fontMetrics(),Qt::Horizontal,0);
            const auto vertical=groundTrackAxisLabels(area,-90,90,canvas.fontMetrics(),Qt::Vertical,4);
            if (horizontal.isEmpty() || vertical.isEmpty()) passed=false;
            else { checkLabels(horizontal,canvas.rect()); checkLabels(vertical,canvas.rect()); }
            for (const auto &x : horizontal) for (const auto &y : vertical)
               require(!x.bounds.intersects(y.bounds), "Ground longitude and latitude labels overlap");
            const double tolerance=720/area.width()+.01;
            hover(canvas,area.center(),0,0,tolerance);
            hover(canvas,area.topLeft()+QPointF(area.width()*.25,area.height()*.25),-90,45,tolerance);
            canvas.zoomBy(std::log(2.)/std::log(1.15)); canvas.repaint();
            hover(canvas,area.center(),0,0,tolerance);
            const QPointF from=area.center(),to=from+QPointF(10,6);
            QMouseEvent press(QEvent::MouseButtonPress,from,canvas.mapToGlobal(from.toPoint()),Qt::LeftButton,Qt::LeftButton,Qt::ShiftModifier);
            QMouseEvent drag(QEvent::MouseMove,to,canvas.mapToGlobal(to.toPoint()),Qt::NoButton,Qt::LeftButton,Qt::ShiftModifier);
            QMouseEvent release(QEvent::MouseButtonRelease,to,canvas.mapToGlobal(to.toPoint()),Qt::LeftButton,Qt::NoButton,Qt::ShiftModifier);
            QApplication::sendEvent(&canvas,&press); QApplication::sendEvent(&canvas,&drag); QApplication::sendEvent(&canvas,&release);
            canvas.repaint(); hover(canvas,area.center(),-10*180/area.width(),6*90/area.height(),tolerance);
            canvas.fit(); canvas.repaint(); hover(canvas,area.center(),0,0,tolerance);
            require(model->curves[0].points.size()==retained.size(), "Ground view changed retained data");
            for (size_t i=0;i<retained.size();++i)
               require(model->curves[0].points[i].x==retained[i].x && model->curves[0].points[i].y==retained[i].y,
                       "Ground view changed geographic samples");
            observations.append(QJsonObject{{"font_points",fontSize},{"width",canvas.width()},{"height",canvas.height()},
               {"map_width",area.width()},{"map_height",area.height()},{"fills",fills},
               {"longitude_labels",horizontal.size()},{"latitude_labels",vertical.size()}});
         }
         canvas.close();
      }
      const QJsonObject result{{"status",passed ? "passed" : "failed"},{"observations",observations},
                              {"checks","map fill, label bounds/spacing, resize, zoom/pan/fit hover, retained coordinates"}};
      if (!directory.isEmpty()) {
         QFile file(directory+"/proof.json"); require(file.open(QIODevice::WriteOnly),"Ground proof cannot be saved");
         file.write(QJsonDocument(result).toJson());
      }
      std::cout<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return passed ? 0 : 1;
   } catch (const std::exception &error) {
      std::cerr<<error.what()<<std::endl;
      return 1;
   }
}
