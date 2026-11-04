#include "AcceptanceView.hpp"
#include "AcceptancePlatform.hpp"
#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWindow>
#include <QScreen>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (app.arguments().contains("--platform-contract")) {
      const QJsonObject info{{"platform",app.platformName()},
         {"mode",qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")},
         {"native_launch",qEnvironmentVariable("GMAT_QT_NATIVE_TEST_LAUNCH")},
         {"accepted",acceptanceWidgetPlatform()}};
      std::cout<<QJsonDocument(info).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   }
   if (app.arguments().contains("--view-contract")) {
      QWidget hidden;
      try {
         acceptanceView(hidden);
         std::cout<<"visible="<<hidden.isVisible()<<std::endl;
         return 0;
      } catch (const std::exception &error) {
         std::cerr<<error.what()<<std::endl;
         return 3;
      }
   }
   if (app.arguments().contains("--host-display-info")) {
      const auto *screen=app.primaryScreen();
      if (!screen) return 2;
      const auto size=screen->geometry().size(),available=screen->availableGeometry().size();
      const QJsonObject info{{"scale",screen->devicePixelRatio()},
         {"logical_size",QJsonArray{size.width(),size.height()}},
         {"available_logical_size",QJsonArray{available.width(),available.height()}}};
      std::cout<<QJsonDocument(info).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   }
   if (app.arguments().contains("--capture-reuse-contract")) {
      QWidget window; window.setObjectName("captureReuseWindow"); window.resize(960,700);
      auto *layout=new QHBoxLayout(&window);
      auto *label=new QLabel("Known final result = 42",&window);
      label->setObjectName("captureReuseValue"); layout->addWidget(label);
      window.show();
      QTimer::singleShot(80,&window,[&] {
         auto boundary=[&] { AcceptanceViewBoundary event(*label); QCoreApplication::sendEvent(&app,&event); };
         for (int i=0;i<5;++i) boundary();
         label->setText("Known final result = 84");
         for (int i=0;i<5;++i) boundary();
         window.resize(900,650); QCoreApplication::processEvents(); boundary();
         label->setText("Known final result = 42"); boundary();
         app.exit(label->text()=="Known final result = 42" ? 0 : 1);
      });
      return app.exec();
   }
   if (app.arguments().contains("--checkpoint-budget-contract")) {
      QWidget window; window.setObjectName("checkpointBudgetWindow"); window.resize(480,160);
      auto *layout=new QHBoxLayout(&window);
      auto *label=new QLabel("Before result",&window); label->setObjectName("checkpointBudgetValue"); layout->addWidget(label);
      window.show();
      QTimer::singleShot(80,&window,[&] {
         if (!window.windowHandle() || !window.windowHandle()->isExposed()) { app.exit(3); return; }
         for (int i=0;i<1100;++i) {
            label->setText(i==1099 ? "Known final result = 42" : QString::number(i));
            AcceptanceViewBoundary boundary(*label); QCoreApplication::sendEvent(&app,&boundary);
         }
         std::cout<<"budget_boundaries=1100 final="<<label->text().toStdString()<<std::endl;
         app.exit(label->text()=="Known final result = 42" ? 0 : 1);
      });
      return app.exec();
   }
   if (app.arguments().contains("--boundary-contract")) {
      QWidget window; window.setObjectName("probeBoundaryWindow"); window.resize(480,160);
      auto *layout=new QHBoxLayout(&window);
      auto *late=new QLabel("Before result",&window); late->setObjectName("probeLateBoundary"); layout->addWidget(late);
      window.show();
      QTimer::singleShot(80,&window,[&] {
         late->setText("Known final result = 42");
         QCoreApplication::processEvents(QEventLoop::AllEvents,20);
         AcceptanceViewBoundary boundary(*late);
         QCoreApplication::sendEvent(&app,&boundary);
         app.exit(late->text()=="Known final result = 42" ? 0 : 1);
      });
      return app.exec();
   }
   QWidget window; window.setObjectName("acceptanceProbe"); window.resize(480,160);
   auto *layout=new QHBoxLayout(&window);
   auto *edit=new QLineEdit(&window); edit->setObjectName("probeEdit"); layout->addWidget(edit);
   auto *combo=new QComboBox(&window); combo->setObjectName("probeChoice"); combo->addItems({"First","Second"}); layout->addWidget(combo);
   auto *button=new QPushButton("Check interaction",&window); button->setObjectName("probeButton"); layout->addWidget(button);
   bool clicked=false;
   QObject::connect(button,&QPushButton::clicked,[&] { clicked=true; });
   window.show();
   QTimer::singleShot(80,&window,[&] {
      edit->setText("checked"); combo->setCurrentIndex(1); button->click();
      QTimer::singleShot(400,&window,[&] {
         std::cout<<"Probe platform="<<app.platformName().toStdString()<<" visible="<<window.isVisible()
            <<" exposed="<<(window.windowHandle() && window.windowHandle()->isExposed())<<std::endl;
         app.exit(clicked && edit->text()=="checked" && combo->currentIndex()==1 ? 0 : 1);
      });
   });
   return app.exec();
}
