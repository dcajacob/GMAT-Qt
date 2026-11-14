#pragma once
#include <QJsonObject>
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include <memory>
#include <vector>

// Test-only GUI input. Every authoring operation is a QtTest input event.
// Selectors identify live controls; recorded screen coordinates are rejected.
class TutorialQtTestDriver final
{
public:
   TutorialQtTestDriver(QWidget &root,const QString &directory);
private:
   struct Target {
      QPointer<QWidget> widget;
      QPoint point;
      QString role;
      int index=-1;
      QJsonObject identity;
   };
   struct Frame {
      QString id;
      QPointer<QWidget> modal,popup;
      bool allowPopup=false,popupSeen=false;
   };
   QWidget &root;
   QString directory,last;
   QTimer timer;
   std::vector<std::shared_ptr<Frame>> frames;
   Target locate(const QJsonObject &action);
   void execute(const QJsonObject &action,Target target,const std::shared_ptr<Frame> &frame);
   void poll();
   void record(const QString &path,const QJsonObject &object);
};
