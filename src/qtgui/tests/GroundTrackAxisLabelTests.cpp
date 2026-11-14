#include "GroundTrackLayout.hpp"
#include <QApplication>
#include <iostream>
#include <stdexcept>

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      for (int points:{10,14}) {
         QFont font=app.font(); font.setPointSize(points);
         const QFontMetrics metrics(font);
         for (auto range:{QPair<double,double>{-90.,90.},{15.1273,53.7531},{-0.0034,0.0017},
                          {39.74112,39.74114},{-179.9999,-179.9998}}) {
            const QRectF area(220,10,600,400);
            for (auto orientation:{Qt::Horizontal,Qt::Vertical}) {
               const auto labels=groundTrackAxisLabels(area,range.first,range.second,metrics,orientation,20);
               if (labels.isEmpty()) throw std::runtime_error("Numeric axis labels are missing");
               double previous=range.first-1;
               for (const auto &label:labels) {
                  bool valid=false; const double value=label.text.toDouble(&valid);
                  if (!valid || value<range.first || value>range.second || value<=previous)
                     throw std::runtime_error("Axis labels must be complete, distinct numbers inside the displayed range");
                  if (metrics.horizontalAdvance(label.text)>label.bounds.width()+0.1)
                     throw std::runtime_error("Numeric axis label is clipped");
                  previous=value;
               }
            }
         }
      }
      const QFontMetrics metrics(app.font());
      const QRectF narrow(60,10,120,120);
      const auto labels=groundTrackAxisLabels(narrow,15.1273,53.7531,metrics,Qt::Vertical,20);
      for (const auto &label:labels) {
         bool valid=false; label.text.toDouble(&valid);
         if (!valid) throw std::runtime_error("Narrow panes must retain complete numeric labels");
      }
      std::cout<<"PASS complete numeric map ticks across hemispheres, fractional zoom, narrow panes and font sizes\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
