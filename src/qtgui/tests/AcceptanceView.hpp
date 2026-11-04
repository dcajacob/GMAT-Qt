#pragma once
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPointer>
#include <QThread>
#include <QWidget>
#include <QWindow>
#include <stdexcept>

// A safe semantic boundary can occur long after a window's Show event.
// Dispatch synchronously after painting; the opt-in observer owns capture
// policy/budgets, while executables without it simply ignore the event.
class AcceptanceViewBoundary final : public QEvent
{
public:
   static QEvent::Type eventType()
   {
      static const auto type=static_cast<QEvent::Type>(QEvent::registerEventType());
      return type;
   }
   explicit AcceptanceViewBoundary(QWidget &target) : QEvent(eventType()),widget(&target) {}
   QPointer<QWidget> widget;
};

// Call at an explicit, safe test boundary, before scheduling modal callbacks.
// Ordinary unattended tests retain their original scheduling and visibility.
inline void acceptanceView(QWidget &widget)
{
   if (qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")!="visible") return;
   if (QGuiApplication::platformName()=="offscreen" || QGuiApplication::platformName()=="minimal")
      throw std::runtime_error("Visible acceptance requires a native window platform");
   auto *window=widget.window();
   window->show(); widget.show();
   QElapsedTimer deadline; deadline.start();
   while ((!window->windowHandle() || !window->windowHandle()->isExposed()) && deadline.elapsed()<5000) {
      QCoreApplication::processEvents(QEventLoop::AllEvents,20);
      QThread::msleep(1);
   }
   if (!widget.isVisible() || !window->windowHandle() || !window->windowHandle()->isExposed())
      throw std::runtime_error("Visible acceptance window did not expose");
   // Deliver the observer's deferred exposure capture before the test proceeds.
   QCoreApplication::processEvents(QEventLoop::AllEvents,20);
   AcceptanceViewBoundary boundary(widget);
   QCoreApplication::sendEvent(QCoreApplication::instance(),&boundary);
}
