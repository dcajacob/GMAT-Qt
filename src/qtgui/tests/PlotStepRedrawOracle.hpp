#pragma once
#include "FormationAcceptance.hpp"
#include "PlotWidget.hpp"
#include "QtPlotReceiver.hpp"
#include <QCryptographicHash>
#include <set>

namespace plotredraw {
inline QImage image(PlotCanvas &canvas) {
 canvas.refresh();
 formaccept::events();
 auto frame=canvas.captureImage().convertToFormat(QImage::Format_RGB32);
 formaccept::require(!frame.isNull(),"Plot framebuffer unavailable");
 return frame;
}

inline int different(const QImage &left,const QImage &right) {
 formaccept::require(left.size()==right.size(),"Reference framebuffer size differs");
 int count=0;
 for(int y=0;y<left.height();++y) {
  const auto *a=reinterpret_cast<const QRgb *>(left.constScanLine(y));
  const auto *b=reinterpret_cast<const QRgb *>(right.constScanLine(y));
  for(int x=0;x<left.width();++x)if(a[x]!=b[x])++count;
 }
 return count;
}

inline QString hash(const QImage &frame) {
 const auto bytes=QByteArrayView(reinterpret_cast<const char *>(frame.constBits()),frame.sizeInBytes());
 return QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
}

// Render the real plot, then render counterfactual segment lists in an isolated
// viewer of identical size. Keep all vertices for camera fitting and disable
// decimation on the reference model. Its line list comes from literal counts.
inline QJsonArray inspect(formaccept::Rig &rig,const QString &name,int limit,int retained,bool finished,const QString &solverMode={},bool flushed=false) {
 QJsonArray plots;
 const QString phase=finished?"final":flushed?"flush":"live";
 for(const auto &which:QStringList{"Orb","Map"}) {
  auto *receiver=rig.window.plotReceiver();
  const auto model=receiver->model(which);
  formaccept::require(model&&model->endOfRun==finished&&model->redrawPoints==limit,"Unexpected live/completed plot configuration");
  formaccept::require(receiver->show(which),"Cannot open production plot");
  PlotWidget *widget=nullptr;
  for(auto *window:rig.area->subWindowList())if(window->property("plotName")==which) {
   window->showNormal();rig.area->setActiveSubWindow(window);widget=dynamic_cast<PlotWidget *>(window->widget());
  }
  formaccept::require(widget&&widget->canvas(),"Production plot canvas unavailable");
  formaccept::events();
  auto *actualCanvas=widget->canvas();
  acceptanceView(*actualCanvas);
  const auto actual=image(*actualCanvas);
  auto reference=std::make_shared<PlotModel>(*model);
  reference->redrawPoints=0;
  PlotCurve *curve=nullptr;
  for(auto &c:reference->curves)if(c.name=="One")curve=&c;
  QVector<int> epochs;
  QVector<bool> links,trials;
  if(solverMode.isEmpty()) {
   const int count=std::min(11,retained);
   for(int i=0;i<count;++i) {epochs<<100*(11-count+i);links<<(i!=0);trials<<false;}
  } else if(finished) {
   epochs={1000,1200,1400,1600,1800,2000};links={false,true,true,true,true,true};trials.fill(false,6);
  } else if(solverMode=="None"||(solverMode=="Current"&&which=="Orb"&&!flushed)) {
   epochs={0,200,400};links={false,true,true};trials.fill(false,3);
  } else {
   epochs={200,400,600,800,1000,1200};links={false,true,false,true,true,true};trials={false,false,true,true,true,true};
  }
  const int total=epochs.size();
  QJsonArray observed;
  if(curve)for(const auto &pt:curve->points)
   observed.append(QJsonObject{{"seconds",(pt.epoch-21545)*86400},{"connect",pt.connect},{"trial",pt.solver}});
  if(!solverMode.isEmpty())rig.save(name+"-"+phase+"-"+which+"-arcs.json",QJsonDocument(observed).toJson());
  formaccept::require(curve&&int(curve->points.size())==total,"Unexpected retained redraw sample count");
  for(int i=0;i<total;++i) {
   const auto &point=curve->points[i];
   formaccept::require(std::abs((point.epoch-21545)*86400-epochs[i])<1e-4&&point.connect==links[i]&&point.solver==trials[i],"Unexpected retained redraw arc");
  }
  PlotCanvas canvas(reference,actualCanvas->parentWidget());
  canvas.setGeometry(actualCanvas->geometry());canvas.show();canvas.raise();canvas.setFrame(actualCanvas->frame());
  const int wanted=finished||limit==0?total-1:limit;
  QJsonArray alternatives;
  const std::set<int> limits{wanted,wanted-1,std::min(total-1,wanted+1),0,total-1};
  bool equal=false,controls=true;
  for(int segments:limits) {
   for(int i=0;i<total;++i)curve->points[i].connect=links[i]&&i>=total-segments;
   const auto prediction=image(canvas);
   const int delta=different(actual,prediction);
   alternatives.append(QJsonObject{{"segments",segments},{"different_pixels",delta},{"reference_sha256",hash(prediction)}});
   if(segments==wanted)equal=delta==0;
   else controls=controls&&delta>=5;
  }
  plots.append(QJsonObject{{"plot",which},{"phase",phase},{"width",actual.width()},{"height",actual.height()},{"actual_sha256",hash(actual)},{"retained",total},{"expected_segments",wanted},{"alternatives",alternatives}});
  rig.save(name+"-"+phase+"-pixels.json",QJsonDocument(plots).toJson());
  formaccept::require(equal,"Rendered pixels differ from independent literal segment list");
  formaccept::require(controls,"Incorrect visible segment count was not distinguishable in pixels");
 }
 return plots;
}
}
