#include "GroundTrack.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace {
class Projection : public GroundTrack
{
public:
   Projection(double radius, double flattening) : GroundTrack("GeodeticPrecision")
   {
      bodyRadius = radius;
      bodyFlattening = flattening;
   }
   std::array<double, 2> project(const std::array<double, 6> &state)
   {
      std::array<double, 2> result;
      CalculateLatLong(state.data(), result[0], result[1]);
      return result;
   }
};
}

int main()
{
   // Independent forward ellipsoid geometry, evaluated in long double.
   // The production function solves the inverse problem iteratively.
   const long double radians = std::acos(-1.L) / 180.L;
   const std::array<std::array<double, 2>, 4> shapes{{
      {6378.1363, 0.}, {6378.1363, 1. / 298.257223563},
      {71492., .06487}, {60268., .09796}}};
   double maxLatitudeError = 0., maxLongitudeError = 0.;
   int samples = 0, failures = 0;
   for (const auto &shape : shapes)
   {
      Projection projection(shape[0], shape[1]);
      const long double radius = shape[0], flattening = shape[1];
      const long double eccentricitySquared = flattening * (2.L - flattening);
      for (double latitude : {-90., -75., -45., -11.42940771747977, 0.,
                              11.42940771747977, 45., 75., 90.})
         for (double longitude : {-179., -80., 0., 90., 179.})
            for (double height : {-10., 0., 500., 40000.})
            {
               const long double phi = latitude * radians, lambda = longitude * radians;
               const long double sinPhi = std::sin(phi), cosPhi = std::cos(phi);
               const long double normal = radius /
                  std::sqrt(1.L - eccentricitySquared * sinPhi * sinPhi);
               const std::array<double, 6> state{{
                  double((normal + height) * cosPhi * std::cos(lambda)),
                  double((normal + height) * cosPhi * std::sin(lambda)),
                  double((normal * (1.L - eccentricitySquared) + height) * sinPhi),
                  0., 0., 0.}};
               const auto result = projection.project(state);
               // At the poles longitude is undefined. Latitude remains defined.
               const double latitudeError = std::abs(result[1] - latitude);
               const double longitudeError = std::abs(latitude) == 90. ? 0. :
                  std::abs(std::remainder(result[0] - longitude, 360.));
               maxLatitudeError = std::max(maxLatitudeError, latitudeError);
               maxLongitudeError = std::max(maxLongitudeError, longitudeError);
               ++samples;
               if (!std::isfinite(result[0]) || !std::isfinite(result[1]) ||
                   latitudeError > 1e-9 || longitudeError > 1e-9)
                  ++failures;
            }
   }
   std::cout.precision(17);
   std::cout << (failures ? "FAIL " : "PASS ") << samples
             << " independent ellipsoid projections; failing=" << failures
             << "; maximum latitude error(deg)=" << maxLatitudeError
             << "; maximum longitude error(deg)=" << maxLongitudeError << '\n';
   return failures ? 1 : 0;
}
