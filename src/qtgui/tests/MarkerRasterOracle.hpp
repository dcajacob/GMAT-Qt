#pragma once
// Scalar geometry oracle: no QPainter, native drawing, or plot transforms.
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace markeroracle {
constexpr double pi=3.14159265358979323846;
inline void require(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
inline double norm(QPointF p) { return std::hypot(p.x(),p.y()); }
inline double segmentDistance(QPointF p,QPointF a,QPointF b)
{
   const auto v=b-a,w=p-a;
   const double vv=v.x()*v.x()+v.y()*v.y();
   const double t=std::clamp((w.x()*v.x()+w.y()*v.y())/vv,0.,1.);
   return norm(p-a-t*v);
}
struct Shape
{
   std::vector<std::pair<QPointF,QPointF>> segments;
   bool circle=false;
   explicit Shape(const QJsonObject &spec)
   {
      circle=spec["unit_circle"].toBool();
      for(auto value:spec["segments"].toArray()) {
         const auto a=value.toArray();require(a.size()==4,"Malformed geometric segment");
         segments.push_back({{a[0].toDouble(),a[1].toDouble()},{a[2].toDouble(),a[3].toDouble()}});
      }
      require(circle||!segments.empty(),"Empty semantic marker shape");
   }
   double distance(QPointF p,double radius,double highlightRadius) const
   {
      double result=std::numeric_limits<double>::infinity();
      for(const auto &s:segments)result=std::min(result,segmentDistance(p,s.first*radius,s.second*radius));
      if(circle)result=std::min(result,std::abs(norm(p)-radius));
      if(highlightRadius>0)result=std::min(result,std::abs(norm(p)-highlightRadius));
      return result;
   }
   std::vector<QPointF> witnesses(double radius,double highlightRadius) const
   {
      std::vector<QPointF> result;
      for(const auto &s:segments) {
         const auto a=s.first*radius,b=s.second*radius;
         const int count=std::max(1,int(std::ceil(norm(b-a)/.75)));
         for(int i=0;i<=count;++i)result.push_back(a+(b-a)*(double(i)/count));
      }
      auto ring=[&](double r) {
         const int count=std::max(16,int(std::ceil(2*pi*r/.75)));
         for(int i=0;i<count;++i) { const double a=2*pi*i/count;result.push_back({r*std::cos(a),r*std::sin(a)}); }
      };
      if(circle)ring(radius);if(highlightRadius>0)ring(highlightRadius);
      return result;
   }
};
inline bool ink(QRgb pixel)
{
   // Check the visible stroke (at least 25% red contrast), separately from
   // Qt's faint filter fringe. A direct QImage line diagnostic demonstrates
   // a 12% fringe beyond the ideal cap, even at DPR 1. Full-color counts below
   // retain the original XY oracle; neither its threshold nor test changes.
   // At the frame a correctly layered antialiased red stroke blends with gray,
   // not white. Keep the original XY red floor and reject colored LCD text
   // fringes by requiring the green/blue channels to remain close together.
   return qRed(pixel)>180&&qRed(pixel)-qGreen(pixel)>64&&qRed(pixel)-qBlue(pixel)>64&&
          std::abs(qGreen(pixel)-qBlue(pixel))<=12;
}
struct Metrics { int pixels=0;double maxDistance=0,maxWitness=0,peakAlpha=0;int strongPixels=0,checkedWitnesses=0,isolatedContacts=0,clippedWitnesses=0; };
inline Metrics check(const QImage &image,QPointF center,const Shape &shape,double size,
                     double ratio,bool highlighted,const QRectF &clip,bool expectInk=true)
{
   const double radius=size*ratio/2,ring=highlighted?(size/2+3)*ratio:0;
   const double stroke=std::max(1.,ratio),extent=std::max(radius,ring)+stroke+2;
   const QRect roi=QRectF(center-QPointF(extent,extent),QSizeF(2*extent,2*extent)).toAlignedRect().intersected(image.rect());
   std::vector<QPointF> observed;Metrics m;
   // A square stroke cap is at most half the stroke diagonal from its
   // centerline; a covered pixel center adds half a pixel diagonal. The .25
   // bound allows ellipse flattening/antialias quantization, in physical px.
   const double allowed=(stroke+1)/std::sqrt(2.)+.25;
   for(int y=roi.top();y<=roi.bottom();++y)for(int x=roi.left();x<=roi.right();++x) {
      const auto pixel=image.pixel(x,y);if(!ink(pixel))continue;
      const QPointF p(x+.5,y+.5);
      require(clip.intersects(QRectF(x,y,1,1)),"Marker ink escaped the plot clipping rectangle");
      const double d=shape.distance(p-center,radius,ring);
      m.maxDistance=std::max(m.maxDistance,d);
      if(d>allowed)std::cerr<<"RASTER_GEOMETRY pixel="<<x<<","<<y<<" center="<<center.x()<<","<<center.y()
         <<" radius="<<radius<<" distance="<<d<<" allowed="<<allowed<<std::endl;
      require(d<=allowed,"Marker ink disagrees with independent shape geometry");
      m.peakAlpha=std::max(m.peakAlpha,1.-qGreen(pixel)/255.);observed.push_back(p);++m.pixels;
      if(qRed(pixel)>180&&qGreen(pixel)<100&&qBlue(pixel)<100)++m.strongPixels;
   }
   if(!expectInk) { require(m.pixels==0,"Hidden/outside marker remained visible");return m; }
   require(m.pixels>0,"A visible in-range marker disappeared");
   int checked=0;const auto witnesses=shape.witnesses(radius,ring);
   // Fractional device-pixel clipping can discard the entire boundary pixel.
   // Demand positive centerline witnesses only within the whole-pixel clip.
   // A partially clipped glyph may have no such witnesses; its visible pixels
   // still undergo every geometry/leakage check above and must contain ink.
   const QRectF witnessClip(QPointF(std::ceil(clip.left()),std::ceil(clip.top())),
                            QPointF(std::floor(clip.right()),std::floor(clip.bottom())));
   for(const auto &w:witnesses) {
      const auto target=center+w;if(!clip.contains(target))continue;
      if(!witnessClip.contains(target)) { ++m.clippedWitnesses;continue; }
      // A centerline that only touches a clip corner has zero local length
      // inside it. Its antialiased fragment need not reach the contrast mask.
      // Require nearby centerline support inside the clip (.75px sampling),
      // while still checking every observed pixel and requiring visible ink.
      bool supported=false;
      for(const auto &near:witnesses) {
         const double d=norm(near-w);
         if(d>1e-8&&d<=.8&&clip.contains(center+near)) { supported=true;break; }
      }
      if(!supported) { ++m.isolatedContacts;continue; }
      double nearest=std::numeric_limits<double>::infinity();
      for(auto p:observed)nearest=std::min(nearest,norm(p-target));
      m.maxWitness=std::max(m.maxWitness,nearest);
      if(nearest>stroke/2+1.25)std::cerr<<"RASTER_WITNESS target="<<target.x()<<","<<target.y()
         <<" center="<<center.x()<<","<<center.y()<<" nearest="<<nearest<<" bound="<<stroke/2+1.25<<" ink="<<m.pixels<<std::endl;
      require(nearest<=stroke/2+1.25,"A defining shape feature is missing from the raster");++checked;
   }
   m.checkedWitnesses=checked;
   require(checked>0||m.clippedWitnesses>0,"No supported geometric witness inside plot clip");
   return m;
}
inline QJsonArray json(const Metrics &m)
{ return {m.pixels,m.maxDistance,m.maxWitness,m.peakAlpha,m.strongPixels,m.checkedWitnesses,m.isolatedContacts,m.clippedWitnesses}; }
}
