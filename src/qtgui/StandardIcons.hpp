// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QStyle>
#include <QWidget>
#include <cmath>

namespace GmatQtIcons {

inline double luminance(const QColor &color)
{
   auto linear=[](double channel) {
      return channel<=0.04045 ? channel/12.92 : std::pow((channel+0.055)/1.055,2.4);
   };
   return 0.2126*linear(color.redF())+0.7152*linear(color.greenF())+0.0722*linear(color.blueF());
}

inline double contrast(const QColor &first,const QColor &second)
{
   const double a=luminance(first),b=luminance(second);
   return a>b ? (a+0.05)/(b+0.05) : (b+0.05)/(a+0.05);
}

inline QColor readableColor(QColor color,const QColor &background,double minimum)
{
   color.setAlpha(255);
   if (contrast(color,background)>=minimum) return color;
   const QColor target=contrast(Qt::white,background)>contrast(Qt::black,background) ? Qt::white : Qt::black;
   for (int step=1;step<=100;++step) {
      const double weight=step/100.;
      const auto adjusted=QColor::fromRgbF(
         color.redF()+(target.redF()-color.redF())*weight,
         color.greenF()+(target.greenF()-color.greenF())*weight,
         color.blueF()+(target.blueF()-color.blueF())*weight);
      if (contrast(adjusted,background)>=minimum) return adjusted;
   }
   return target;
}

inline QColor actionColor(QWidget *context,QIcon::Mode mode)
{
   const QPalette palette=context ? context->palette() : QApplication::palette();
   const auto group=mode==QIcon::Disabled ? QPalette::Disabled : QPalette::Active;
   const auto foreground=mode==QIcon::Selected ? QPalette::HighlightedText : QPalette::ButtonText;
   const auto background=mode==QIcon::Selected ? QPalette::Highlight : QPalette::Window;
   return readableColor(palette.color(group,foreground),palette.color(group,background),
                        mode==QIcon::Disabled ? 3.0 : 4.5);
}

// Platform action pixmaps can contain fixed dark pixels and a second disabled
// fade. Draw their silhouettes from the current widget palette. Reading
// the palette for each paint also handles a theme change after icon creation.
class ActionIconEngine final : public QIconEngine
{
public:
   ActionIconEngine(QStyle::StandardPixmap symbol,QWidget *widget)
      : symbol(symbol),context(widget) {}
   QIconEngine *clone() const override { return new ActionIconEngine(symbol,context.data()); }
   QString key() const override { return QStringLiteral("GmatActionIcon"); }
   void paint(QPainter *painter,const QRect &rect,QIcon::Mode mode,QIcon::State) override
   {
      if (rect.isEmpty()) return;
      const QColor color=actionColor(context.data(),mode);
      painter->save();
      painter->setRenderHint(QPainter::Antialiasing);
      const qreal side=qMin(rect.width(),rect.height());
      painter->translate(rect.x()+(rect.width()-side)/2.,rect.y()+(rect.height()-side)/2.);
      painter->scale(side/20.,side/20.);
      painter->setPen(Qt::NoPen); painter->setBrush(color);
      auto triangle=[painter](qreal tip,qreal base) {
         QPainterPath path; path.moveTo(base,4); path.lineTo(tip,10);
         path.lineTo(base,16); path.closeSubpath(); painter->drawPath(path);
      };
      switch (symbol) {
      case QStyle::SP_MediaPlay: triangle(16,6); break;
      case QStyle::SP_MediaPause:
         painter->drawRect(QRectF(5,4,3.5,12)); painter->drawRect(QRectF(11.5,4,3.5,12)); break;
      case QStyle::SP_MediaStop: painter->drawRect(QRectF(4.5,4.5,11,11)); break;
      case QStyle::SP_MediaSkipBackward:
         painter->drawRect(QRectF(4,4,2,12)); triangle(7,15); break;
      case QStyle::SP_MediaSkipForward:
         triangle(13,5); painter->drawRect(QRectF(14,4,2,12)); break;
      case QStyle::SP_MediaSeekBackward: triangle(3,11); triangle(10,18); break;
      case QStyle::SP_MediaSeekForward: triangle(10,2); triangle(17,9); break;
      case QStyle::SP_ArrowLeft: triangle(4,15); break;
      case QStyle::SP_ArrowRight: triangle(16,5); break;
      case QStyle::SP_TitleBarMinButton:
         painter->setRenderHint(QPainter::Antialiasing,false);
         painter->drawRect(QRectF(4,13,12,2)); break;
      case QStyle::SP_TitleBarMaxButton: {
         painter->setRenderHint(QPainter::Antialiasing,false);
         QPainterPath shape;
         shape.addRect(QRectF(4,4,12,12));
         shape.addRect(QRectF(6,7,8,7));
         painter->drawPath(shape); break;
      }
      case QStyle::SP_TitleBarNormalButton: {
         painter->setRenderHint(QPainter::Antialiasing,false);
         painter->drawRect(QRectF(7,3,10,2));
         painter->drawRect(QRectF(15,5,2,8));
         painter->drawRect(QRectF(13,11,2,2));
         painter->drawRect(QRectF(7,5,2,2));
         QPainterPath shape;
         shape.addRect(QRectF(3,7,10,10));
         shape.addRect(QRectF(5,10,6,5));
         painter->drawPath(shape); break;
      }
      case QStyle::SP_TitleBarCloseButton:
         painter->setPen(QPen(color,2,Qt::SolidLine,Qt::SquareCap));
         painter->drawLine(QPointF(5,5),QPointF(15,15));
         painter->drawLine(QPointF(15,5),QPointF(5,15)); break;
      default: break;
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
   QStyle::StandardPixmap symbol;
   QPointer<QWidget> context;
};
}

inline QIcon standardActionIcon(QStyle::StandardPixmap symbol,QWidget *context)
{
   switch (symbol) {
   case QStyle::SP_MediaPlay: case QStyle::SP_MediaPause: case QStyle::SP_MediaStop:
   case QStyle::SP_MediaSkipBackward: case QStyle::SP_MediaSkipForward:
   case QStyle::SP_MediaSeekBackward: case QStyle::SP_MediaSeekForward:
   case QStyle::SP_ArrowLeft: case QStyle::SP_ArrowRight:
   case QStyle::SP_TitleBarMinButton: case QStyle::SP_TitleBarMaxButton:
   case QStyle::SP_TitleBarNormalButton: case QStyle::SP_TitleBarCloseButton:
      return QIcon(new GmatQtIcons::ActionIconEngine(symbol,context));
   default:
      return (context ? context->style() : QApplication::style())->standardIcon(symbol,nullptr,context);
   }
}
