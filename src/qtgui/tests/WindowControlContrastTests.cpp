#include "WorkspaceTabs.hpp"
#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QImage>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QStyleFactory>
#include <QStyleOptionTitleBar>
#include <QTest>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition,const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

void events()
{
   QApplication::processEvents();
   QTest::qWait(30);
}

double luminance(const QColor &color)
{
   const auto linear=[](double c) { return c<=0.04045 ? c/12.92 : std::pow((c+0.055)/1.055,2.4); };
   return 0.2126*linear(color.redF())+0.7152*linear(color.greenF())+0.0722*linear(color.blueF());
}

void checkGlyph(const QImage &image,const QColor &foreground,const QColor &background)
{
   const double a=luminance(foreground),b=luminance(background);
   require((std::max(a,b)+0.05)/(std::min(a,b)+0.05)>=4.5,"Fixture foreground lacks contrast");
   int matching=0;
   for (int y=0;y<image.height();++y) for (int x=0;x<image.width();++x) {
      const auto color=image.pixelColor(x,y);
      if (color.alpha()<200) continue;
      // Small native buttons have antialiased strokes without fully covered pixels.
      const double coverage=(color.red()-background.red())/double(foreground.red()-background.red());
      if (coverage<0.65 || coverage>1.03) continue;
      if (std::abs(color.green()-(background.green()+coverage*(foreground.green()-background.green())))<=5 &&
          std::abs(color.blue()-(background.blue()+coverage*(foreground.blue()-background.blue())))<=5)
         ++matching;
   }
   require(matching>=4,"Window control has no readable palette-colored glyph");
}

QStyleOptionTitleBar titleOption(QMdiSubWindow *window)
{
   QStyleOptionTitleBar option;
   option.initFrom(window);
   option.titleBarFlags=window->windowFlags();
   option.titleBarState=window->windowState();
   option.rect=QRect(0,0,window->width(),window->style()->pixelMetric(QStyle::PM_TitleBarHeight,&option,window));
   return option;
}

QRect controlRect(QMdiSubWindow *window,QStyle::SubControl control)
{
   const auto option=titleOption(window);
   return window->style()->subControlRect(QStyle::CC_TitleBar,&option,control,window);
}

void nativeGlyphs(QMdiSubWindow *window,const QColor &foreground,const QColor &background)
{
   const auto controls=window->isMinimized()
      ? QList<QStyle::SubControl>{QStyle::SC_TitleBarNormalButton,QStyle::SC_TitleBarMaxButton,QStyle::SC_TitleBarCloseButton}
      : QList<QStyle::SubControl>{QStyle::SC_TitleBarMinButton,QStyle::SC_TitleBarMaxButton,QStyle::SC_TitleBarCloseButton};
   const auto title=window->grab(titleOption(window).rect).toImage();
   for (auto control:controls) {
      const auto rect=controlRect(window,control);
      require(!rect.isEmpty(),"Native window control is missing");
      const qreal ratio=title.devicePixelRatio();
      const QRect pixels(qRound(rect.x()*ratio),qRound(rect.y()*ratio),qRound(rect.width()*ratio),qRound(rect.height()*ratio));
      checkGlyph(title.copy(pixels),foreground,background);
   }
}

void clickNative(QMdiSubWindow *window,QStyle::SubControl control)
{
   const auto rect=controlRect(window,control);
   require(!rect.isEmpty(),"Native click target is missing");
   QTest::mouseMove(window,rect.center());
   events();
   QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,rect.center());
   events();
}
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      const QString styleName=argc>1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("Fusion");
      require(QStyleFactory::keys().contains(styleName,Qt::CaseInsensitive),"Requested style is unavailable");
      app.setStyle(styleName);
      QWidget host;
      auto *layout=new QVBoxLayout(&host);
      auto *area=new QMdiArea(&host);
      auto *header=new QHBoxLayout;
      auto *tabs=new WorkspaceTabs(area,&host);
      header->addWidget(tabs,1);
      header->addWidget(tabs->windowControls());
      layout->addLayout(header);
      layout->addWidget(area);
      auto *first=area->addSubWindow(new QWidget);
      auto *second=area->addSubWindow(new QWidget);
      first->setWindowTitle("First window"); second->setWindowTitle("Second window");
      first->setAttribute(Qt::WA_DeleteOnClose,false);
      second->setAttribute(Qt::WA_DeleteOnClose,false);
      host.resize(900,550);
      host.show(); first->show(); second->show(); events();
      const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      QDir().mkpath(directory);
      for (bool dark:{true,false,true}) {
         const QColor background=dark ? QColor(40,40,40) : QColor(242,242,242);
         const QColor foreground=dark ? QColor(235,240,210) : QColor(30,32,48);
         QPalette palette=app.palette();
         for (auto group:{QPalette::Active,QPalette::Inactive,QPalette::Disabled}) {
            for (auto role:{QPalette::Window,QPalette::Button,QPalette::Base,QPalette::Highlight})
               palette.setColor(group,role,background);
            for (auto role:{QPalette::WindowText,QPalette::ButtonText,QPalette::Text,QPalette::HighlightedText})
               palette.setColor(group,role,foreground);
         }
         app.setPalette(palette); events();
         first->showNormal(); second->showNormal();
         first->setGeometry(0,0,400,350); second->setGeometry(420,0,400,350);
         area->setActiveSubWindow(second); events();
         if (!directory.isEmpty()) host.grab().save(directory+"/"+styleName+(dark ? "-dark" : "-light")+".png");
         nativeGlyphs(first,foreground,background);
         nativeGlyphs(second,foreground,background);
         QTest::mouseMove(second,controlRect(second,QStyle::SC_TitleBarCloseButton).center()); events();
         nativeGlyphs(second,foreground,background);
         if (!directory.isEmpty()) host.grab().save(directory+"/"+styleName+(dark ? "-dark" : "-light")+".png");
         clickNative(second,QStyle::SC_TitleBarMinButton);
         require(second->isMinimized(),"Native minimize failed");
         nativeGlyphs(second,foreground,background);
         clickNative(second,QStyle::SC_TitleBarNormalButton);
         require(!second->isMinimized(),"Native restore failed");
         clickNative(second,QStyle::SC_TitleBarMaxButton);
         require(second->isMaximized(),"Native maximize failed");
         auto *minimize=tabs->windowControls()->findChild<QToolButton *>("workspaceMinimizeWindow");
         auto *restore=tabs->windowControls()->findChild<QToolButton *>("workspaceRestoreWindow");
         auto *close=tabs->windowControls()->findChild<QToolButton *>("workspaceCloseWindow");
         require(minimize && restore && close && tabs->windowControls()->isVisible(),"Maximized controls are missing");
         for (auto *button:{minimize,restore,close}) {
            for (auto mode:{QIcon::Normal,QIcon::Active,QIcon::Selected,QIcon::Disabled})
               for (int size:{16,20,40}) checkGlyph(button->icon().pixmap(size,size,mode).toImage(),foreground,background);
            require(!button->toolTip().isEmpty() && !button->accessibleName().isEmpty(),"Control lacks a label");
            checkGlyph(button->grab().toImage(),foreground,background);
         }
         QTest::mouseClick(restore,Qt::LeftButton); events();
         require(!second->isMaximized(),"Header restore failed");
         second->showMaximized(); events();
         QTest::mouseClick(minimize,Qt::LeftButton); events();
         require(second->isMinimized(),"Header minimize failed");
         second->showMaximized(); events();
         QTest::mouseClick(close,Qt::LeftButton); events();
         require(second->isHidden(),"Header close failed");
         second->showNormal(); events();
         clickNative(second,QStyle::SC_TitleBarCloseButton);
         require(second->isHidden(),"Native close failed");
         second->showNormal(); events();
      }
      std::cout<<"PASS "<<styleName.toStdString()<<" native and maximized controls, inactive/active/hover, light/dark live changes, icon modes/sizes and minimize/maximize/restore/close\n";
      return 0;
   } catch (const std::exception &error) {
      std::cerr<<error.what()<<'\n'; return 1;
   }
}
