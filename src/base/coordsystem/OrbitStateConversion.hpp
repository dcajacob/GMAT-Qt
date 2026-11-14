// SPDX-License-Identifier: Apache-2.0
// Coordinate context for state models whose force symmetry has a fixed pole.
#ifndef OrbitStateConversion_hpp
#define OrbitStateConversion_hpp

#include "CoordinateSystem.hpp"
#include "CoordinateConverter.hpp"
#include "CelestialBody.hpp"
#include "StateConversionUtil.hpp"
#include "UtilityException.hpp"
#include "GmatConstants.hpp"
#include <cmath>
#include <memory>

namespace OrbitStateConversion
{
inline bool IsBrouwer(const std::string &type)
{
   return type == "BrouwerMeanShort" || type == "BrouwerMeanLong";
}

// The scalar Brouwer formulas use the fixed JGM-2 Earth pole in MJ2000Eq.
// Rotating only the input Cartesian state would instead rotate the gravity
// model. A mean ellipse is transported between bases through a Keplerian/MA
// Cartesian proxy, with the zonal corrections applied in their reference axes.
template<class Epoch>
Rvector6 Convert(const Epoch &epoch, const Rvector6 &input,
      const std::string &from, const std::string &to, CoordinateSystem *frame,
      Real mu, Real flattening, Real radius, const std::string &anomaly = "TA")
{
   if ((!IsBrouwer(from) && !IsBrouwer(to)) || from == to || !frame || !frame->GetOrigin())
      return StateConversionUtil::Convert(input, from, to, mu, flattening, radius, anomaly);
   auto *body = dynamic_cast<CelestialBody *>(frame->GetOrigin());
   if (!body || body->GetPhysicalBodyName() != "Earth")
      throw UtilityException("Brouwer mean states require the Earth as their physical origin.\n");
   if (frame->AreAxesOfType("MJ2000EqAxes"))
      return StateConversionUtil::Convert(input, from, to, mu, flattening, radius, anomaly);

   auto *earth = frame->GetOrigin();
   std::unique_ptr<CoordinateSystem> reference(CoordinateSystem::CreateLocalCoordinateSystem(
         "BrouwerReference", "MJ2000Eq", earth, nullptr, nullptr, earth, frame->GetSolarSystem()));
   if (!reference)
      throw UtilityException("Cannot initialize the Earth reference axes for Brouwer conversion.\n");
   CoordinateConverter converter;
   const bool inertial = frame->AreAxesOfType("InertialAxes");
   Rvector6 osculatingReference, inReference;
   if (IsBrouwer(from))
   {
      Rvector6 normalized = input;
      for (Integer i = 0; i < 6; ++i)
         if (!std::isfinite(input[i]))
            throw UtilityException("Brouwer mean elements must be finite.\n");
      if (input[0] <= 0.0 || std::abs(input[1]) > 0.99 || input[2] < 0.0 || input[2] > 180.0 ||
            input[0]*(1.0-std::abs(input[1])) < 3000.0)
         throw UtilityException("Brouwer mean elements require a positive SMA, ECC below 0.99, "
               "inclination within 0..180 degrees and periapsis above 3000 km.\n");
      if (normalized[1] < 0.0)
      {
         // Preserve the native negative-eccentricity apoapsis convention.
         normalized[1] = -normalized[1];
         normalized[4] += 180.0;
         normalized[5] -= 180.0;
      }
      const Rvector6 meanInFrame = StateConversionUtil::Convert(normalized, "Keplerian", "Cartesian",
            mu, flattening, radius, "MA");
      converter.Convert(epoch, meanInFrame, frame, inReference, reference.get(), false, true);
      Rvector6 meanReference = StateConversionUtil::Convert(inReference, "Cartesian", "Keplerian",
            mu, flattening, radius, "MA");
      if (inertial)
      {
         meanReference[0] = normalized[0];
         meanReference[1] = normalized[1];
         if (normalized[1] > GmatOrbitConstants::KEP_TOL) meanReference[5] = normalized[5];
      }
      osculatingReference = StateConversionUtil::Convert(meanReference, from, "Cartesian",
            mu, flattening, radius);
   }
   else
   {
      const Rvector6 inFrame = StateConversionUtil::Convert(input, from, "Cartesian",
            mu, flattening, radius, anomaly);
      converter.Convert(epoch, inFrame, frame, osculatingReference, reference.get(), false, true);
   }
   Rvector6 inFrame;
   if (IsBrouwer(to))
   {
      const Rvector6 meanReference = StateConversionUtil::Convert(osculatingReference,
            "Cartesian", to, mu, flattening, radius);
      const Rvector6 meanCartesian = StateConversionUtil::Convert(meanReference, "Keplerian", "Cartesian",
            mu, flattening, radius, "MA");
      converter.Convert(epoch, meanCartesian, reference.get(), inFrame, frame, false, true);
      Rvector6 result = StateConversionUtil::Convert(inFrame, "Cartesian", "Keplerian",
            mu, flattening, radius, "MA");
      if (inertial)
      {
         result[0] = meanReference[0];
         result[1] = meanReference[1];
         if (meanReference[1] > GmatOrbitConstants::KEP_TOL) result[5] = meanReference[5];
      }
      return result;
   }
   converter.Convert(epoch, osculatingReference, reference.get(), inFrame, frame, false, true);
   return StateConversionUtil::Convert(inFrame, "Cartesian", to, mu, flattening, radius, anomaly);
}
}

#endif
