#include "WorkspaceTabs.hpp"
#include "WorkspaceLayout.hpp"
#include "StandardIcons.hpp"
#include <QEvent>
#include <QHBoxLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QProxyStyle>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleOptionTitleBar>
#include <QTimer>
#include <QToolButton>

namespace {
// Some native styles paint inactive title-bar symbols with a fixed black pen.
// Retain their frame, title, geometry and hit testing, and paint these four
// controls with the same palette-aware icons as the workspace header.
class MdiWindowStyle final : public QProxyStyle
{
public:
   MdiWindowStyle() : QProxyStyle(QApplication::style()->objectName()) {}

   void drawComplexControl(ComplexControl control,const QStyleOptionComplex *option,
                           QPainter *painter,const QWidget *widget) const override
   {
      const auto *title=qstyleoption_cast<const QStyleOptionTitleBar *>(option);
      if (control!=CC_TitleBar || !title || !qobject_cast<const QMdiSubWindow *>(widget)) {
         QProxyStyle::drawComplexControl(control,option,painter,widget);
         return;
      }
      auto background=*title;
      // QMdiSubWindow may retain title colors from before a palette change.
      // Resolve them from the widget just as the action icon engine does.
      if (widget) background.palette=widget->palette();
      background.subControls&=~(SC_TitleBarMinButton|SC_TitleBarMaxButton|
                                SC_TitleBarNormalButton|SC_TitleBarCloseButton);
      QProxyStyle::drawComplexControl(control,&background,painter,widget);
      const bool active=title->titleBarState & State_Active;
      const bool enabled=title->state & State_Enabled;
      const auto mode=enabled ? (active ? QIcon::Selected : QIcon::Normal) : QIcon::Disabled;
      const struct { SubControl control; StandardPixmap icon; } buttons[]={
         {SC_TitleBarMinButton,SP_TitleBarMinButton},
         {SC_TitleBarMaxButton,SP_TitleBarMaxButton},
         {SC_TitleBarNormalButton,SP_TitleBarNormalButton},
         {SC_TitleBarCloseButton,SP_TitleBarCloseButton}
      };
      for (const auto &button:buttons) {
         if (!(title->subControls & button.control)) continue;
         const QRect rect=subControlRect(CC_TitleBar,title,button.control,widget);
         if (rect.isEmpty()) continue;
         QStyleOption panel=*title;
         panel.rect=rect;
         panel.palette=background.palette;
         panel.state=enabled ? State_Enabled : State_None;
         const bool hovered=enabled && (title->activeSubControls & button.control);
         if (hovered) {
            panel.state|=(title->state & State_Sunken) ? State_Sunken : State_Raised;
            if (active) panel.palette.setColor(QPalette::Button,background.palette.color(QPalette::Highlight));
            drawPrimitive(PE_PanelButtonTool,&panel,painter,widget);
         }
         const int side=qMin(16,qMin(rect.width(),rect.height())-2);
         QRect iconRect(0,0,side,side);
         iconRect.moveCenter(rect.center());
         standardActionIcon(button.icon,const_cast<QWidget *>(widget)).paint(painter,iconRect,Qt::AlignCenter,mode);
      }
   }
};
}

WorkspaceTabs::WorkspaceTabs(QMdiArea *workspace, QWidget *parent) : QTabBar(parent), area(workspace)
{
   // MDI activation requires a shared style when maximized children are switched.
   if (!dynamic_cast<MdiWindowStyle *>(QApplication::style()))
      QApplication::setStyle(new MdiWindowStyle);
   setObjectName("workspaceTabs");
   setAccessibleName("Open windows");
   setDocumentMode(true);
   setExpanding(false);
   setUsesScrollButtons(true);
   setElideMode(Qt::ElideRight);
   controls=new QWidget(this);
   controls->setObjectName("workspaceWindowControls");
   auto *layout=new QHBoxLayout(controls);
   layout->setContentsMargins(0,0,0,0);
   layout->setSpacing(0);
   const auto button=[&](const QString &name,const QString &label,QStyle::StandardPixmap icon) {
      auto *result=new QToolButton(controls);
      result->setObjectName(name);
      result->setIcon(standardActionIcon(icon,result));
      result->setToolTip(label);
      result->setAccessibleName(label);
      result->setAutoRaise(true);
      result->setFocusPolicy(Qt::NoFocus);
      layout->addWidget(result);
      return result;
   };
   minimize=button("workspaceMinimizeWindow","Minimize window",QStyle::SP_TitleBarMinButton);
   restore=button("workspaceRestoreWindow","Restore window",QStyle::SP_TitleBarNormalButton);
   closeWindow=button("workspaceCloseWindow","Close window",QStyle::SP_TitleBarCloseButton);
   connect(minimize,&QToolButton::clicked,this,[this] { if (auto *window=area->activeSubWindow()) window->showMinimized(); });
   connect(restore,&QToolButton::clicked,this,[this] { if (auto *window=area->activeSubWindow()) window->showNormal(); });
   connect(closeWindow,&QToolButton::clicked,this,[this] { if (auto *window=area->activeSubWindow()) window->close(); });
   controls->hide();
   area->viewport()->installEventFilter(this);
   area->installEventFilter(this);
   connect(area,&QMdiArea::subWindowActivated,this,[this] { scheduleRefresh(); });
   connect(this,&QTabBar::currentChanged,this,[this](int index) { activate(index); });
   connect(this,&QTabBar::tabBarClicked,this,[this](int index) { activate(index); });
   hide();
   scheduleRefresh();
}

void WorkspaceTabs::scheduleRefresh()
{
   if (refreshPending) return;
   refreshPending=true;
   QTimer::singleShot(0,this,[this] { refreshPending=false; refresh(); });
}

bool WorkspaceTabs::eventFilter(QObject *watched, QEvent *event)
{
   switch (event->type()) {
   case QEvent::ChildAdded:
   case QEvent::ChildRemoved:
   case QEvent::Show:
   case QEvent::Hide:
   case QEvent::Close:
   case QEvent::WindowStateChange:
      scheduleRefresh();
      break;
   default:
      break;
   }
   return QTabBar::eventFilter(watched,event);
}

void WorkspaceTabs::refresh()
{
   const QSignalBlocker blocker(this);
   while (count()) removeTab(count()-1);
   windows.clear();
   for (auto *window:area->subWindowList(QMdiArea::CreationOrder)) {
      if (!observed.contains(window)) {
         observed.insert(window);
         window->installEventFilter(this);
         connect(window,&QWidget::windowTitleChanged,this,[this] { scheduleRefresh(); });
         connect(window,&QObject::destroyed,this,[this,window] { observed.remove(window); scheduleRefresh(); });
      }
      if (auto *buttons=window->maximizedButtonsWidget()) buttons->hide();
      if (auto *icon=window->maximizedSystemMenuIconWidget()) icon->hide();
      if (window->isHidden()) continue;
      windows.append(window);
      const int index=addTab(window->windowTitle());
      setTabToolTip(index,window->windowTitle());
      if (window==area->activeSubWindow()) setCurrentIndex(index);
   }
   auto *active=area->activeSubWindow();
   controls->setVisible(active && active->isMaximized() && !active->isMinimized() && !active->isHidden());
   if (active) {
      minimize->setEnabled(active->windowFlags().testFlag(Qt::WindowMinimizeButtonHint));
      restore->setEnabled(active->windowFlags().testFlag(Qt::WindowMaximizeButtonHint));
      closeWindow->setEnabled(active->windowFlags().testFlag(Qt::WindowCloseButtonHint));
   }
   setVisible(!windows.isEmpty());
}

void WorkspaceTabs::activate(int index)
{
   if (index<0 || index>=windows.size() || !windows[index]) return;
   auto *window=windows[index].data();
   window->showMaximized();
   activateWorkspaceWindow(area,window);
}
