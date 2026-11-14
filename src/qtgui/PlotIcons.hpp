// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StandardIcons.hpp"

namespace GmatQtIcons {

enum class PlotIcon { Fit, Camera, Display, ScriptView, ResetView, Sunlight, Style, ExportData, SaveImage, More, ReplayMode, Layers };

// Use the same palette, canvas and single-color silhouettes as transport icons.
class PlotIconEngine final : public QIconEngine
{
public:
   PlotIconEngine(PlotIcon symbol,QWidget *widget) : symbol(symbol),context(widget) {}
   QIconEngine *clone() const override { return new PlotIconEngine(symbol,context.data()); }
   QString key() const override { return QStringLiteral("GmatPlotIcon"); }
   void paint(QPainter *painter,const QRect &rect,QIcon::Mode mode,QIcon::State) override
   {
      if (rect.isEmpty()) return;
      const QColor color=actionColor(context.data(),mode);
      painter->save();
      painter->setRenderHint(QPainter::Antialiasing);
      const qreal side=qMin(rect.width(),rect.height());
      painter->translate(rect.x()+(rect.width()-side)/2.,rect.y()+(rect.height()-side)/2.);
      painter->scale(side/20.,side/20.);
      painter->setPen(Qt::NoPen);
      painter->setBrush(color);
      QPainterPath shape;
      switch (symbol) {
      case PlotIcon::Fit:
         // Opposing corner arrows keep the fit action legible at small sizes.
         for (int corner=0;corner<2;++corner) {
            painter->save();
            painter->translate(10,10);
            painter->rotate(corner*180);
            QPainterPath arrow;
            arrow.moveTo(-7,-7); arrow.lineTo(-1,-7); arrow.lineTo(-3,-5);
            arrow.lineTo(-.5,-2.5); arrow.lineTo(-2.5,-.5); arrow.lineTo(-5,-3);
            arrow.lineTo(-7,-1); arrow.closeSubpath();
            painter->drawPath(arrow);
            painter->restore();
         }
         break;
      case PlotIcon::Camera:
         shape.addRoundedRect(QRectF(2,5,16,12),1.5,1.5);
         shape.addEllipse(QRectF(6,7,8,8));
         painter->drawPath(shape);
         painter->drawRoundedRect(QRectF(6,2.5,8,3.5),1,1);
         painter->drawEllipse(QRectF(8,9,4,4));
         break;
      case PlotIcon::Display:
         shape.addRoundedRect(QRectF(2,3,16,11.5),1,1);
         shape.addRect(QRectF(4,5,12,7.5));
         painter->drawPath(shape);
         painter->drawRect(QRectF(9,14,2,2));
         painter->drawRoundedRect(QRectF(5.5,16,9,2),.5,.5);
         break;
      case PlotIcon::ScriptView:
         shape.moveTo(4,2); shape.lineTo(12,2); shape.lineTo(17,7);
         shape.lineTo(17,18); shape.lineTo(4,18); shape.closeSubpath();
         shape.moveTo(12,3.5); shape.lineTo(12,7); shape.lineTo(15.5,7); shape.closeSubpath();
         shape.moveTo(9,8.5); shape.lineTo(6,11.5); shape.lineTo(9,14.5);
         shape.lineTo(10,13.5); shape.lineTo(8,11.5); shape.lineTo(10,9.5); shape.closeSubpath();
         shape.moveTo(12,8.5); shape.lineTo(15,11.5); shape.lineTo(12,14.5);
         shape.lineTo(11,13.5); shape.lineTo(13,11.5); shape.lineTo(11,9.5); shape.closeSubpath();
         painter->drawPath(shape);
         break;
      case PlotIcon::ResetView:
         shape.moveTo(2,4); shape.lineTo(8,4); shape.lineTo(2,10); shape.closeSubpath();
         painter->drawPath(shape);
         painter->setBrush(Qt::NoBrush);
         painter->setPen(QPen(color,2.5,Qt::SolidLine,Qt::RoundCap));
         painter->drawArc(QRectF(4,4,12,12),0,-285*16);
         break;
      case PlotIcon::Sunlight:
         painter->drawEllipse(QRectF(6,6,8,8));
         for (int ray=0;ray<8;++ray) {
            painter->save(); painter->translate(10,10); painter->rotate(ray*45);
            painter->drawRoundedRect(QRectF(-1,-9,2,3),.5,.5); painter->restore();
         }
         break;
      case PlotIcon::Style:
         painter->drawRect(QRectF(3,4,14,2));
         painter->drawRect(QRectF(3,9,14,2));
         painter->drawRect(QRectF(3,14,14,2));
         painter->drawEllipse(QRectF(5,2.5,5,5));
         painter->drawEllipse(QRectF(11,7.5,5,5));
         painter->drawEllipse(QRectF(4,12.5,5,5));
         break;
      case PlotIcon::ExportData:
         shape.addRect(QRectF(2,3,12,14));
         shape.addRect(QRectF(4,6,3,3)); shape.addRect(QRectF(9,6,3,3));
         shape.addRect(QRectF(4,11,3,4)); shape.addRect(QRectF(9,11,3,4));
         painter->drawPath(shape);
         shape=QPainterPath();
         shape.moveTo(15,9); shape.lineTo(19,13); shape.lineTo(15,17);
         shape.lineTo(15,14); shape.lineTo(11,14); shape.lineTo(11,12);
         shape.lineTo(15,12); shape.closeSubpath();
         painter->drawPath(shape);
         break;
      case PlotIcon::SaveImage:
         shape.moveTo(8.5,2); shape.lineTo(11.5,2); shape.lineTo(11.5,9);
         shape.lineTo(15,9); shape.lineTo(10,14); shape.lineTo(5,9);
         shape.lineTo(8.5,9); shape.closeSubpath();
         painter->drawPath(shape);
         painter->drawRect(QRectF(3,13,2,5)); painter->drawRect(QRectF(15,13,2,5));
         painter->drawRect(QRectF(3,16,14,2));
         break;
      case PlotIcon::More:
         painter->drawEllipse(QRectF(2,8.5,3,3));
         painter->drawEllipse(QRectF(8.5,8.5,3,3));
         painter->drawEllipse(QRectF(15,8.5,3,3));
         break;
      case PlotIcon::ReplayMode:
         painter->drawEllipse(QRectF(2,3,3,3)); painter->drawRect(QRectF(7,3.5,11,2));
         painter->drawEllipse(QRectF(2,8.5,3,3)); painter->drawRect(QRectF(7,9,8,2));
         painter->drawEllipse(QRectF(2,14,3,3)); painter->drawRect(QRectF(7,14.5,5,2));
         break;
      case PlotIcon::Layers:
         shape.moveTo(2,6); shape.lineTo(10,2); shape.lineTo(18,6); shape.lineTo(10,10); shape.closeSubpath();
         painter->drawPath(shape);
         shape=QPainterPath();
         shape.moveTo(2,10); shape.lineTo(4,9); shape.lineTo(10,12); shape.lineTo(16,9);
         shape.lineTo(18,10); shape.lineTo(10,14); shape.closeSubpath(); painter->drawPath(shape);
         shape=QPainterPath();
         shape.moveTo(2,14); shape.lineTo(4,13); shape.lineTo(10,16); shape.lineTo(16,13);
         shape.lineTo(18,14); shape.lineTo(10,18); shape.closeSubpath(); painter->drawPath(shape);
         break;
      }
      painter->restore();
   }
   QPixmap pixmap(const QSize &size,QIcon::Mode mode,QIcon::State state) override
   {
      if (size.isEmpty()) return {};
      QPixmap result(size); result.fill(Qt::transparent);
      QPainter painter(&result); paint(&painter,QRect(QPoint(),size),mode,state);
      return result;
   }
private:
   PlotIcon symbol;
   QPointer<QWidget> context;
};
}

inline QIcon plotActionIcon(GmatQtIcons::PlotIcon symbol,QWidget *context)
{
   return QIcon(new GmatQtIcons::PlotIconEngine(symbol,context));
}
