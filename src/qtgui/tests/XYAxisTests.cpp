#include "MainWindow.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"XY report unavailable"); return f.readAll(); }
static void run(MainWindow &window) { require(window.runMission()==MainWindow::RunResult::Completed,"XY fixture mission failed"); }
static bool colored(const QImage &image,QPoint at) {
   for (int y=at.y()-3;y<=at.y()+3;++y) for (int x=at.x()-3;x<=at.x()+3;++x) if (image.rect().contains(x,y)) { const auto c=image.pixelColor(x,y); if (c.red()>180 && c.green()<100 && c.blue()<100) return true; } return false;
}
// The independent fixture coordinates below are QWidget logical pixels.
// captureImage intentionally exports physical pixels, including its DPR.
static QImage logicalCapture(PlotCanvas &canvas) {
   auto physical=canvas.captureImage();
   const auto ratio=canvas.devicePixelRatioF();
   require(!physical.isNull() && std::abs(physical.devicePixelRatio()-ratio)<1e-6,
      "XY capture lost its device-pixel ratio");
   require(physical.width()==qRound(canvas.width()*ratio) && physical.height()==qRound(canvas.height()*ratio),
      "XY capture dimensions do not match physical display pixels");
   auto logical=physical.size()==canvas.size() ? physical : physical.scaled(canvas.size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
   logical.setDevicePixelRatio(1);
   return logical;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtXYAxes");
   try {
      const bool preview=argc==4 && QString::fromLocal8Bit(argv[2])=="--preview"; TestSettings settings; QTemporaryDir files; require((argc==2 || preview) && files.isValid(),"XY axis setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"XY axis runtime unavailable"); auto *receiver=window.plotReceiver(); auto *area=window.findChild<QMdiArea *>("workspace");
      if (preview) {
         receiver->CreateXyPlotWindow("AxisPreview","",0,0,0,0,false,"XY axis options","Elapsed seconds","Value",true); receiver->AddXyPlotCurve("AxisPreview",0,"Curve",0xff0000); receiver->UpdateXyPlotCurve("AxisPreview",0,1,2); require(receiver->show("AxisPreview"),"Native XY preview missing"); auto *plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget());
         std::exception_ptr failure; QTimer::singleShot(0,plot,[&] { auto *dialog=plot->findChild<QDialog *>("plotStyleDialog"); try { require(dialog,"Native XY style dialog missing"); auto *tabs=dialog->findChild<QTabWidget *>("plotStyleTabs");
            for (int index=0;index<tabs->count();++index) { tabs->setCurrentIndex(index); QEventLoop wait; QTimer::singleShot(100,&wait,&QEventLoop::quit); wait.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed(),"Native XY controls not exposed"); for (auto *field:tabs->currentWidget()->findChildren<QWidget *>()) if ((dynamic_cast<QLineEdit *>(field) || dynamic_cast<QSpinBox *>(field)) && field->isVisible()) require(dialog->rect().contains(field->mapTo(dialog,field->rect().bottomRight())),"Native XY field clipped"); require(dialog->grab().save(QString::fromLocal8Bit(argv[3])+"."+QString::number(index)+".png"),"Native XY capture failed"); }
         } catch (...) { failure=std::current_exception(); } if (dialog) dialog->reject(); }); plot->findChild<QAction *>("plotStyleAction")->trigger(); if (failure) std::rethrow_exception(failure); std::cout<<"PASS: native actual MDI XY Plot/X-axis/Y-axis/Curves controls exposed and captured, with no numerical execution.\n"; return 0;
      }
      const auto report=files.filePath("axis report.txt"),saved=files.filePath("XY axes Δ.script"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString source="% preserve display-independent source α\nCreate Variable X Y;\nCreate XYPlot Chart;\nChart.XVariable = X;\nChart.YVariables = {Y};\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nFor X = 0:10;\nY = X^2;\nReport Values X Y;\nEndFor;\n";
      editor->setPlainText(source); run(window); const auto expected=read(report); require(!expected.isEmpty() && receiver->show("Chart"),"Actual XY plot/report missing"); auto *child=area->activeSubWindow(); auto *plot=dynamic_cast<PlotWidget *>(child->widget()); require(plot,"XY MDI widget missing"); auto model=receiver->model("Chart"); const auto points=model->curves[0].points.size(); const auto before=plot->canvas()->captureImage();
      auto edit=[&](std::function<void(QDialog *)> operation) { std::exception_ptr failure; QTimer::singleShot(0,plot,[&] { auto *dialog=plot->findChild<QDialog *>("plotStyleDialog"); try { require(dialog,"XY style dialog missing"); operation(dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); plot->findChild<QAction *>("plotStyleAction")->trigger(); if (failure) std::rethrow_exception(failure); };
      auto limit=[](QDialog *d,const QString &axis,const QString &side,const QString &value,bool enabled=true) { d->findChild<QCheckBox *>("plot"+axis+"Axis"+side+"Enabled")->setChecked(enabled); d->findChild<QLineEdit *>("plot"+axis+"Axis"+side)->setText(value); };
      auto configure=[&](QDialog *d) { d->findChild<QLineEdit *>("plotTitle")->setText("Edited XY title Δ"); d->findChild<QLineEdit *>("plotXLabel")->setText("Seconds"); d->findChild<QLineEdit *>("plotYLabel")->setText("Square"); limit(d,"X","Minimum","-2"); limit(d,"X","Maximum","20"); limit(d,"Y","Minimum","-10"); limit(d,"Y","Maximum","150"); d->findChild<QSpinBox *>("plotXAxisTicks")->setValue(8); d->findChild<QSpinBox *>("plotYAxisTicks")->setValue(3); d->findChild<QSpinBox *>("plotXAxisPrecision")->setValue(10); d->findChild<QSpinBox *>("plotYAxisPrecision")->setValue(12); };
      edit([&](QDialog *d) { configure(d); d->reject(); }); require(plot->canvas()->captureImage()==before && !model->xAxis.minimum && model->title!="Edited XY title Δ","XY Cancel changed display");
      edit([&](QDialog *d) { configure(d); auto *ok=d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok); limit(d,"X","Minimum","20"); require(!ok->isEnabled() && !d->findChild<QLabel *>("plotAxisStatus")->text().isEmpty(),"Equal axis limits accepted"); for (const auto &bad:QStringList{"nan","inf","1e101","oops"}) { limit(d,"X","Minimum",bad); require(!ok->isEnabled(),"Invalid axis limit accepted"); } limit(d,"X","Minimum","-2"); require(ok->isEnabled(),"Corrected axis limit rejected"); ok->click(); });
      require(model->title=="Edited XY title Δ" && model->xLabel=="Seconds" && model->yLabel=="Square" && model->xAxis.minimum==-2 && model->xAxis.maximum==20 && model->yAxis.minimum==-10 && model->yAxis.maximum==150 && model->xAxis.ticks==8 && model->yAxis.ticks==3 && model->xAxis.precision==10 && model->yAxis.precision==12,"XY axis/label controls not applied"); require(plot->canvas()->captureImage()!=before && model->curves[0].points.size()==points && editor->toPlainText()==source && read(report)==expected,"Display edits changed samples/source/report or did not render");
      child->close(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(receiver->show("Chart") && receiver->model("Chart").get()==model.get(),"XY close/reopen lost model"); plot=dynamic_cast<PlotWidget *>(area->activeSubWindow()->widget());
      edit([&](QDialog *d) { require(d->findChild<QLineEdit *>("plotTitle")->text()==model->title && d->findChild<QLineEdit *>("plotXAxisMinimum")->text()=="-2" && d->findChild<QSpinBox *>("plotYAxisPrecision")->value()==12,"Reopened XY controls lost values"); limit(d,"X","Minimum","200"); limit(d,"X","Maximum","0",false); limit(d,"Y","Minimum","0",false); limit(d,"Y","Maximum","-200"); require(d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->isEnabled(),"Independent one-sided limits rejected"); d->accept(); }); require(!plot->canvas()->captureImage().isNull(),"One-sided out-of-data axis limits failed to render");
      edit([&](QDialog *d) { for (const auto &axis:QStringList{"X","Y"}) for (const auto &side:QStringList{"Minimum","Maximum"}) limit(d,axis,side,"0",false); d->accept(); }); require(!model->xAxis.minimum && !model->xAxis.maximum && !model->yAxis.minimum && !model->yAxis.maximum,"Automatic axis ranges not restored");
      require(window.saveScriptTo(saved) && read(saved)==source.toUtf8() && window.loadScript(saved),"XY display changes altered saved/reopened script"); run(window); require(read(report)==expected && !receiver->model("Chart")->xAxis.minimum && receiver->model("Chart")->title!="Edited XY title Δ","Rebuild/rerun did not reset display settings or changed calculation");
      // Independent screen geometry: x=5 is one quarter across [0,20],
      // y=5 is halfway up [0,10], with legend/grid off. Data must clip at bounds.
      auto fixture=std::make_shared<PlotModel>(PlotModel::Kind::XY); fixture->grid=false; fixture->legend=false; fixture->labels=false; fixture->curves[0].color=Qt::red; fixture->curves[0].lines=false; fixture->curves[0].markers=true; fixture->curves[0].markerType=4; fixture->curves[0].markerSize=8; fixture->append(0,5,5); fixture->xAxis.minimum=0; fixture->xAxis.maximum=20; fixture->yAxis.minimum=0; fixture->yAxis.maximum=10;
      PlotCanvas canvas(fixture); canvas.resize(600,400); canvas.show(); QApplication::processEvents(); auto image=logicalCapture(canvas); require(colored(image,{199,188}),"Fixed XY range did not place sample at independent screen coordinate"); fixture->xAxis.maximum=10; canvas.refresh(); QApplication::processEvents(); require(colored(logicalCapture(canvas),{324,188}),"Changed XY range did not relocate rendered sample"); fixture->yAxis.maximum=4; canvas.refresh(); QApplication::processEvents(); image=logicalCapture(canvas); bool red=false; for (int y=32;y<344;++y) for (int x=74;x<574;++x) red|=colored(image,{x,y}); require(!red,"Out-of-range XY sample was not clipped");
      fixture->yAxis.maximum=10; fixture->grid=true; fixture->xAxis.ticks=5; fixture->yAxis.ticks=5; canvas.refresh(); QApplication::processEvents(); const auto oldGrid=logicalCapture(canvas); fixture->xAxis.ticks=2; fixture->yAxis.ticks=3; canvas.refresh(); QApplication::processEvents(); const auto newGrid=logicalCapture(canvas); require(newGrid.pixelColor(324,80)!=oldGrid.pixelColor(324,80),"X tick setting did not change grid placement"); require(newGrid.pixelColor(120,240)!=oldGrid.pixelColor(120,240),"Y tick setting did not change grid placement");
      fixture->xAxis.minimum=0.123456789; fixture->xAxis.maximum=10.87654321; fixture->xAxis.precision=2; canvas.refresh(); QApplication::processEvents(); const auto shortLabels=logicalCapture(canvas).copy(0,345,600,24); fixture->xAxis.precision=12; canvas.refresh(); QApplication::processEvents(); require(logicalCapture(canvas).copy(0,345,600,24)!=shortLabels,"X label precision did not change rendered numbers"); fixture->yAxis.minimum=0.123456789; fixture->yAxis.maximum=10.87654321; fixture->yAxis.precision=2; canvas.refresh(); QApplication::processEvents(); const auto shortY=logicalCapture(canvas).copy(24,32,45,312); fixture->yAxis.precision=12; canvas.refresh(); QApplication::processEvents(); require(logicalCapture(canvas).copy(24,32,45,312)!=shortY,"Y label precision did not change rendered numbers");
      std::cout<<"PASS: actual MDI XY labels/ranges/ticks/precision, Cancel/invalid correction, one-sided/automatic ranges, retained samples/close-reopen and exact script/report save-reopen/rerun; independent rendered sample coordinates and clipping.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
