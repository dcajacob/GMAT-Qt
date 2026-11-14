#pragma once
#include <QPointer>
#include <QSet>
#include <QTabBar>
#include <QVector>
class QMdiArea;
class QMdiSubWindow;
class QToolButton;

class WorkspaceTabs final : public QTabBar
{
public:
   explicit WorkspaceTabs(QMdiArea *area, QWidget *parent=nullptr);
   QWidget *windowControls() const { return controls; }
protected:
   bool eventFilter(QObject *watched, QEvent *event) override;
private:
   QMdiArea *area;
   QVector<QPointer<QMdiSubWindow>> windows;
   QSet<QMdiSubWindow *> observed;
   QWidget *controls;
   QToolButton *minimize;
   QToolButton *restore;
   QToolButton *closeWindow;
   bool refreshPending=false;
   void scheduleRefresh();
   void refresh();
   void activate(int index);
};
