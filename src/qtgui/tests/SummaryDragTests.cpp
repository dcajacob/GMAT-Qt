#include "InspectionDialog.hpp"
#include "PlotWidget.hpp"
#include "OrbitCamera.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDir>
#include <QScrollBar>
#include <QTest>
#include <QWheelEvent>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char *message)
{
   if (!value) throw std::runtime_error(message);
}
void events() { QApplication::processEvents(); QTest::qWait(30); }
void summaryScroll()
{
   QString contents;
   for (int line=0;line<400;++line)
      contents+=QString("State %1    %2\n").arg(line).arg(QString(180,'x'));
   InspectionDialog dialog("Mission summary",contents,{},nullptr);
   dialog.resize(700,450); dialog.show(); events();
   auto *text=dialog.findChild<QPlainTextEdit *>("inspectionText");
   require(text,"Summary text is missing");
   text->verticalScrollBar()->setValue(100);
   text->horizontalScrollBar()->setValue(80);
   const QPointF position=text->viewport()->rect().center();
   QWheelEvent wheel(position,text->viewport()->mapToGlobal(position.toPoint()),{},QPoint(0,-120),
      Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
   QApplication::sendEvent(text->viewport(),&wheel); events();
   const int vertical=text->verticalScrollBar()->value();
   const int horizontal=text->horizontalScrollBar()->value();
   require(vertical>100,"Mouse wheel did not scroll the summary");
   for (int i=0;i<4;++i) { dialog.setText(contents); events(); }
   require(text->verticalScrollBar()->value()==vertical && text->horizontalScrollBar()->value()==horizontal,
      "Refreshing identical summary text reset the viewport");
   dialog.setText(contents+"Additional state\n"); events();
   require(text->verticalScrollBar()->value()==vertical && text->horizontalScrollBar()->value()==horizontal,
      "Refreshing changed summary text reset the viewport");
   require(text->toPlainText()==contents+"Additional state\n","Summary refresh lost contents");
   std::cout<<"PASS wheel-scrolled summary retains its viewport across repeated and changed refreshes\n";
}
void drag(PlotCanvas &canvas,QPoint offset,Qt::KeyboardModifiers modifiers=Qt::NoModifier)
{
   const auto center=canvas.rect().center();
   QTest::mousePress(&canvas,Qt::LeftButton,modifiers,center);
   QMouseEvent move(QEvent::MouseMove,center+offset,canvas.mapToGlobal(center+offset),
      Qt::NoButton,Qt::LeftButton,modifiers);
   QApplication::sendEvent(&canvas,&move);
   QTest::mouseRelease(&canvas,Qt::LeftButton,modifiers,center+offset); events();
}
OrbitCameraBasis camera(const PlotModel &model,const PlotCanvas &canvas)
{
   const auto &state=*model.userView;
   return orbitCamera(model,canvas.frame(),state.yaw,state.pitch,1,1,nullptr,nullptr,
      state.orbitRotation ? &*state.orbitRotation : nullptr);
}
QPointF project(const OrbitCameraBasis &camera,const osg::Vec3d &point)
{
   const auto relative=point-camera.target;
   return {relative*camera.right,-(relative*camera.up)};
}
void checkLocalAxes(PlotCanvas &canvas,PlotModel &model)
{
   for (const auto offset:{QPoint(24,0),QPoint(0,18),QPoint(-24,0),QPoint(0,-18)}) {
      const auto before=camera(model,canvas);
      const auto marker=before.target+before.outward;
      const auto start=project(before,marker);
      drag(canvas,offset);
      const auto after=camera(model,canvas);
      const auto motion=project(after,marker)-start;
      require(motion.x()*offset.x()+motion.y()*offset.y()>0,"Rendered near surface moved against the drag");
      require(offset.x() ? std::abs(motion.y())<1e-10 : std::abs(motion.x())<1e-10,
         "Dragging on one screen axis rotated the other screen axis");
      require(std::abs(after.right*after.up)<1e-12 && std::abs(after.right*after.outward)<1e-12 &&
         std::abs(after.up*after.outward)<1e-12 && std::abs(after.outward.length()-1)<1e-12,
         "Camera rotation lost its orthonormal basis");
   }
}
QPointF redCenter(const QImage &image)
{
   double x=0,y=0;int count=0;
   for (int row=0;row<image.height();++row) for (int col=0;col<image.width();++col) {
      const auto color=image.pixelColor(col,row);
      if (color.red()>150 && color.green()<70 && color.blue()<70) { x+=col;y+=row;++count; }
   }
   require(count>10,"Rendered near-surface marker is missing");
   return {x/count,y/count};
}
void orbitDrag()
{
   for (const bool scripted:{false,true}) for (const auto angles:{QPointF(0,0),QPointF(.9,-.7),QPointF(2.4,2.0)}) {
      auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
      model->scriptedCamera=scripted;
      model->cameras.push_back({0,{4,-3,6},{0,0,0},{0,0,1},false});
      PlotCanvas canvas(model);canvas.resize(600,400);canvas.show();events();
      canvas.setViewAngles(angles.x(),angles.y());
      checkLocalAxes(canvas,*model);
      drag(canvas,{0,400}); // Cross a pole and verify axes still follow the screen.
      checkLocalAxes(canvas,*model);
      const auto saved=*model->userView;
      const auto before=camera(*model,canvas);
      drag(canvas,{17,11},Qt::ShiftModifier);
      require(model->userView->pan==saved.pan+QPointF(17,11) && model->userView->orbitRotation==saved.orbitRotation,
         "Shift-drag changed rotation or reversed pan");
      canvas.restoreView(saved);
      const auto restored=camera(*model,canvas);
      require((restored.outward-before.outward).length()<1e-12 && (restored.up-before.up).length()<1e-12,
         "Restoring the view lost the free camera rotation");
      PlotCanvas reopened(model);reopened.resize(600,400);reopened.show();events();
      const auto reopenedCamera=camera(*model,reopened);
      require((reopenedCamera.outward-before.outward).length()<1e-12 && (reopenedCamera.up-before.up).length()<1e-12,
         "Reopening the view lost camera rotation");
      canvas.resetView();require(!model->userView->orbitRotation,"Reset retained a pointer rotation");
   }
   auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);
   model->scriptedCamera=true;model->sunlight=false;model->labels=false;model->legend=false;model->axes=false;model->grid=false;
   model->cameras.push_back({0,{0,0,6},{0,0,0},{0,1,0},false});
   model->curves[0].radius=1;model->curves[0].color=Qt::green;model->append(0,0,0,0);
   model->curves[1].radius=.08;model->curves[1].color=Qt::red;model->append(1,0,0,1.05);
   PlotCanvas canvas(model);canvas.resize(640,480);canvas.show();events();
   const auto initial=canvas.captureImage();const auto start=redCenter(initial);
   drag(canvas,{24,0});const auto horizontal=canvas.captureImage();
   require(redCenter(horizontal).x()>start.x()+1,"Rendered pixels moved left during a right drag");
   canvas.scriptView();events();drag(canvas,{0,18});const auto vertical=canvas.captureImage();
   require(redCenter(vertical).y()>start.y()+1,"Rendered pixels moved up during a downward drag");
   const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   if (!directory.isEmpty()) { QDir().mkpath(directory);initial.save(directory+"/initial.png");horizontal.save(directory+"/right-drag.png");vertical.save(directory+"/down-drag.png"); }
   std::cout<<"PASS camera-local drag in scripted and fallback views, tilted and inverted views, pole crossing, restored views, Shift-pan and rendered pixel direction\n";
}
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      require(argc==2,"Supply summary or drag");
      if (QString::fromLocal8Bit(argv[1])=="summary") summaryScroll(); else orbitDrag();
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
