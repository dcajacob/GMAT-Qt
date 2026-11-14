// SPDX-License-Identifier: Apache-2.0
#ifndef EphemerisDerivative_hpp
#define EphemerisDerivative_hpp

#include "gmatdefs.hpp"
#include "Rvector3.hpp"
#include "SolarSystemException.hpp"
#include <cmath>
#include <exception>
#include <string>

// Internal, same-thread derivative queries. This guard bypasses every body's
// update-interval cache on the call stack without changing its configuration.
namespace GmatEphemerisDetail {
class GMAT_API ScopedDerivativeQuery
{
public:
   ScopedDerivativeQuery();
   ~ScopedDerivativeQuery();
   static bool IsActive();
   ScopedDerivativeQuery(const ScopedDerivativeQuery&) = delete;
   ScopedDerivativeQuery& operator=(const ScopedDerivativeQuery&) = delete;
};

// The callback supplies MJ2000 velocity at a time offset in seconds. Preserve
// the body-provider fourth-order, one-second stencil and coverage-boundary
// fallback. Every sample must exist; no extrapolation or zero fallback is used.
template<class Velocity>
Rvector3 AccelerationFromVelocity(Velocity velocity, const std::string &name)
{
   ScopedDerivativeQuery uncached;
   auto sample = [&](Real seconds)
   {
      const Rvector3 value = velocity(seconds);
      for (Integer i = 0; i < 3; ++i)
         if (!std::isfinite(value[i]))
            throw SolarSystemException("Nonfinite ephemeris velocity in acceleration query for " + name);
      return value;
   };
   Rvector3 acceleration;
   try
   {
      const Rvector3 center = sample(0.0);
      bool computed = false;
      std::exception_ptr centralFailure;
      try
      {
         const Rvector3 m2 = sample(-2.0), m1 = sample(-1.0);
         const Rvector3 p1 = sample(1.0), p2 = sample(2.0);
         acceleration = (8.0 * (p1 - m1) - (p2 - m2)) / 12.0;
         computed = true;
      }
      catch (BaseException &) { centralFailure = std::current_exception(); }
      if (!computed)
      {
         try
         {
            const Rvector3 p1 = sample(1.0), p2 = sample(2.0);
            const Rvector3 p3 = sample(3.0), p4 = sample(4.0);
            acceleration = (-25.0 * center + 48.0 * p1 - 36.0 * p2 + 16.0 * p3 - 3.0 * p4) / 12.0;
            computed = true;
         }
         catch (BaseException &) {}
      }
      if (!computed)
      {
         try
         {
            const Rvector3 m1 = sample(-1.0), m2 = sample(-2.0);
            const Rvector3 m3 = sample(-3.0), m4 = sample(-4.0);
            acceleration = (25.0 * center - 48.0 * m1 + 36.0 * m2 - 16.0 * m3 + 3.0 * m4) / 12.0;
            computed = true;
         }
         catch (BaseException &) {}
      }
      if (!computed) std::rethrow_exception(centralFailure);
      sample(0.0); // Restore provider epoch caches to the requested time.
      for (Integer i = 0; i < 3; ++i)
         if (!std::isfinite(acceleration[i]))
            throw SolarSystemException("Nonfinite MJ2000 acceleration for " + name);
   }
   catch (...)
   {
      try { sample(0.0); } catch (...) {}
      throw;
   }
   return acceleration;
}
}
#endif
