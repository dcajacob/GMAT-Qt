#pragma once
#include "OrbitCamera.hpp"
#include "OrbitObjectAxes.hpp"
#include <QColor>
#include <optional>

struct OrbitSkyStar
{
   std::array<double,3> direction;
   double pixels=1;
   QColor color=Qt::white;
};
inline double orbitStarRawDiameter(double magnitude)
{
   const double luminance=std::pow(100.,-magnitude/5.);
   return 2*std::sqrt(luminance/(std::pow(luminance,2./3.)+std::pow(100.,-.4)));
}
// OF's Ballesteros temperature and RGB approximation. The catalog's optional
// B-V index affects only styled stars; ordinary Qt stars retain their grays.
inline QColor orbitStarColor(double colorIndex,double dimming)
{
   double temperature=46*(1/(.92*colorIndex+1.7)+1/(.92*colorIndex+.62));
   if (!std::isfinite(temperature) || temperature<=0) temperature=46*(1/1.7+1/.62);
   const double red=temperature<=66 ? 255 : 329.698727446*std::pow(temperature-60,-.1332047592);
   const double green=temperature<=66 ? 99.4708025861*std::log(temperature)-161.1195681661 : 288.1221695283*std::pow(temperature-60,-.0755148492);
   const double blue=temperature>=66 ? 255 : temperature<=19 ? 0 : 138.5177312231*std::log(temperature-10)-305.0447927307;
   return QColor::fromRgbF(std::clamp(red/255.,0.,1.)*dimming,std::clamp(green/255.,0.,1.)*dimming,std::clamp(blue/255.,0.,1.)*dimming);
}
inline QVector<OrbitSkyStar> orbitSkyStars(const PlotModel &model)
{
   QVector<OrbitSkyStar> result;
   if (model.printBackground || !model.starsEnabled || model.starCount<=0) return result;
   const double largest=model.starOptions ? orbitStarRawDiameter(model.starOptions->minMagnitude) : 0;
   const double smallest=model.starOptions ? orbitStarRawDiameter(model.starOptions->maxMagnitude) : 0;
   for (const auto &star:model.starCatalog.stars) {
      OrbitSkyStar visible; visible.direction=star.direction;
      if (model.starOptions) {
         const auto &options=*model.starOptions;
         if (star.magnitude<options.minMagnitude || star.magnitude>options.maxMagnitude) continue;
         const double ratio=largest==smallest ? 0 : std::clamp((orbitStarRawDiameter(star.magnitude)-smallest)/(largest-smallest),0.,1.);
         visible.pixels=options.minPixels+ratio*(options.maxPixels-options.minPixels);
         visible.color=orbitStarColor(star.colorIndex,std::max(options.minDimRatio,ratio));
      } else {
         const int group=static_cast<int>(std::clamp((star.magnitude+1)/2,0.,4.));
         visible.pixels=2.8-.45*group; const double brightness=1-.14*group;
         visible.color=QColor::fromRgbF(brightness,brightness,brightness);
      }
      result.append(visible); if (result.size()>=model.starCount) break;
   }
   return result;
}
inline std::array<double,9> orbitSkyFrame(const PlotModel &model,quint64 frame)
{
   const PlotPoint *sample=nullptr;
   for (const auto &curve:model.curves) if (const auto *pose=orbitObjectPose(curve,frame); pose && (!sample || pose->frame>=sample->frame)) sample=pose;
   return sample ? sample->inertialToView : std::array<double,9>{1,0,0,0,1,0,0,0,1};
}
inline osg::Vec3d orbitSkyDirection(const std::array<double,3> &direction,const std::array<double,9> &rotation)
{
   osg::Vec3d result;
   for (int row=0;row<3;++row) for (int col=0;col<3;++col) result[row]+=rotation[row*3+col]*direction[col];
   return result;
}
inline std::optional<QPointF> orbitSkyProjection(const osg::Vec3d &direction,const OrbitCameraBasis &camera,double tangent,double aspect)
{
   const double forward=-(direction*camera.outward);
   if (forward<=0) return {};
   const QPointF projected((direction*camera.right)/(forward*tangent*aspect),(direction*camera.up)/(forward*tangent));
   if (!std::isfinite(projected.x()) || !std::isfinite(projected.y()) || std::abs(projected.x())>1 || std::abs(projected.y())>1) return {};
   return projected;
}
