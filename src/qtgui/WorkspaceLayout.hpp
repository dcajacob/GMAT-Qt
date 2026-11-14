#pragma once
#include <QFontMetrics>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QScrollBar>
#include <QSettings>
#include <QStyle>
#include <QVector>
#include <algorithm>

inline int workspaceColumnLimit()
{
   bool valid=false;
   const int value=QSettings().value("Workspace/maximumColumns",2).toInt(&valid);
   return valid && value>0 ? value : 2;
}

inline QVector<QRect> workspaceTileRectangles(QMdiArea *area, const QList<QMdiSubWindow *> &windows,
                                             int columnLimit=workspaceColumnLimit())
{
   if (windows.isEmpty()) return {};
   const int count=static_cast<int>(windows.size());
   const int fontHeight=QFontMetrics(area->font()).height();
   int minimumWidth=std::max(480,30*fontHeight), minimumHeight=std::max(360,22*fontHeight);
   for (auto *window:windows) {
      const QSize minimum=window->minimumSize().expandedTo(window->minimumSizeHint());
      minimumWidth=std::max(minimumWidth,minimum.width());
      minimumHeight=std::max(minimumHeight,minimum.height());
   }
   int availableWidth=area->viewport()->width();
   const int availableHeight=area->viewport()->height();
   const int preferred=std::min(std::max(1,columnLimit),count);
   auto columnsForWidth=[&] { return std::max(1,std::min(preferred,availableWidth/minimumWidth)); };
   int columns=columnsForWidth();
   if (minimumHeight>availableHeight && !area->verticalScrollBar()->isVisible()) {
      availableWidth-=area->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
      columns=columnsForWidth();
   }
   const int width=std::max(availableWidth,columns*minimumWidth);
   const int height=std::max(availableHeight,minimumHeight);
   QVector<QRect> rectangles;
   for (int column=0; column<columns; ++column) {
      const int left=column*width/columns, right=(column+1)*width/columns;
      rectangles.append(QRect(left,0,right-left,height));
   }
   return rectangles;
}

inline void tileWorkspaceWindows(QMdiArea *area, const QList<QMdiSubWindow *> &candidates,
                                 int columnLimit=workspaceColumnLimit())
{
   area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
   area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
   QList<QMdiSubWindow *> windows;
   const auto history=area->subWindowList(QMdiArea::ActivationHistoryOrder);
   for (auto it=history.crbegin();it!=history.crend();++it) {
      auto *window=*it;
      if (candidates.contains(window) && !window->isHidden() && !window->isMinimized())
         windows.append(window);
   }
   if (windows.isEmpty()) return;
   const bool dontMaximizeOnActivation=area->testOption(QMdiArea::DontMaximizeSubWindowOnActivation);
   area->setOption(QMdiArea::DontMaximizeSubWindowOnActivation,true);
   // Snapshot eligibility before changing states, since MDI activation can
   // otherwise restore a window while another maximized window is normalized.
   for (auto *window:windows) if (window->isMaximized()) window->showNormal();
   const auto rectangles=workspaceTileRectangles(area,windows,columnLimit);
   for (int index=rectangles.size();index<windows.size();++index) windows[index]->showMinimized();
   area->horizontalScrollBar()->setValue(0);
   area->verticalScrollBar()->setValue(0);
   for (int index=0; index<rectangles.size(); ++index) windows[index]->setGeometry(rectangles[index]);
   area->setActiveSubWindow(windows.first());
   windows.first()->raise();
   area->setOption(QMdiArea::DontMaximizeSubWindowOnActivation,dontMaximizeOnActivation);
}

inline void tileWorkspaceWindows(QMdiArea *area, int columnLimit=workspaceColumnLimit())
{
   tileWorkspaceWindows(area,area->subWindowList(),columnLimit);
}

inline void activateWorkspaceWindow(QMdiArea *area, QMdiSubWindow *window)
{
   if (!window) return;
   if (window->isMinimized()) window->showNormal();
   area->setActiveSubWindow(window);
   window->raise();
   auto reveal=[](QScrollBar *bar, int start, int length, int extent) {
      if (start<0 || length>extent) bar->setValue(bar->value()+start);
      else if (start+length>extent) bar->setValue(bar->value()+start+length-extent);
   };
   reveal(area->horizontalScrollBar(),window->x(),window->width(),area->viewport()->width());
   reveal(area->verticalScrollBar(),window->y(),window->height(),area->viewport()->height());
   if (window->widget()) window->widget()->setFocus();
}
