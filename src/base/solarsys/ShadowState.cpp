//$Id$
//------------------------------------------------------------------------------
//                               ShadowState
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); 
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either 
// express or implied.   See the License for the specific language
// governing permissions and limitations under the License.
//
// Author: Wendy C. Shoan
// Created: 2014.05.06
//
/**
 * Base class implementation for the ShadowState.
 */
//------------------------------------------------------------------------------

#include "ShadowState.hpp"
#include "SolarSystem.hpp"
#include "GmatConstants.hpp"
#include "GmatDefaults.hpp"
#include "SolarSystemException.hpp"
#include "RealUtilities.hpp"
#include "MessageInterface.hpp"
#include <algorithm>
#include <cmath>
#include <array>

namespace
{
struct PenumbraGeometry
{
   const ShadowState *owner;
   const Real *state;
   std::array<long double,3> sunVector;
   Real sunRadius;
   Real bodyRadius;
   Real bodyAngle;
   Real sunAngle;
   PenumbraGeometry *previous;

   PenumbraGeometry(const ShadowState *object, const Real *position, const Real *sun,
         const Real *bodyToSun,
         Real solarRadius, Real occultingRadius, Real apparentBody, Real apparentSun) :
      owner(object), state(position), sunRadius(solarRadius),
      bodyRadius(occultingRadius), bodyAngle(apparentBody), sunAngle(apparentSun), previous(Active())
   {
      bool closed = true;
      for (Integer i=0;i<3;++i)
         closed = closed && position[i] - bodyToSun[i] == sun[i];
      for (Integer i=0;i<3;++i)
         sunVector[i] = closed ? static_cast<long double>(position[i]) - bodyToSun[i] : sun[i];
      Active() = this;
   }
   ~PenumbraGeometry() { Active() = previous; }
   static PenumbraGeometry *&Active()
   {
      static thread_local PenumbraGeometry *geometry = nullptr;
      return geometry;
   }
};
}

//#define DEBUG_SHADOW_STATE
//#define DEBUG_SHADOW_STATE1
//#define DEBUG_SHADOW_STATE_INIT
//#define DEBUG_SHADOW_STATE_SUN_VECTOR
//#define DEBUG_SHADOW_STATE_2


//------------------------------------------------------------------------------
// static data
//------------------------------------------------------------------------------
// none at this time

//------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// default constructor
//------------------------------------------------------------------------------
ShadowState::ShadowState() :
   solarSystem  (NULL),
   sun          (NULL)//,
{
}

//------------------------------------------------------------------------------
// copy constructor
//------------------------------------------------------------------------------
ShadowState::ShadowState(const ShadowState &copy) :
   solarSystem  (NULL),
   sun          (NULL)//,
{
}

//------------------------------------------------------------------------------
// operator=
//------------------------------------------------------------------------------
ShadowState& ShadowState::operator=(const ShadowState &copy)
{
   if (&copy != this)
   {
      solarSystem  = NULL;
      sun          = NULL;
   }

   return *this;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
ShadowState::~ShadowState()
{
   solarSystem  = NULL;
   sun          = NULL;
}

//------------------------------------------------------------------------------
// void SetSolarSystem(SolarSystem *ss)
//------------------------------------------------------------------------------
void ShadowState::SetSolarSystem(SolarSystem *ss)
{
   #ifdef DEBUG_SHADOW_STATE_INIT
      MessageInterface::ShowMessage("Setting solar system on ShadowState\n");
   #endif
   solarSystem  = ss;
   sun          = solarSystem->GetBody(GmatSolarSystemDefaults::SUN_NAME);
}



//------------------------------------------------------------------------------
// Real ShadowState::FindShadowState(Real *state,  Real *cbSun,
//                                   Real* sunSat, Real sunRad,
//                                   Real bodyRad)
//------------------------------------------------------------------------------
/**
 * Determines lighting conditions at the input location
 *
 * @param state    Current spacecraft state relative to its Shadow Body
 * @param cbSun    Shadow Body-to-Sun vector
 * @param sunSat   Sun-to-SC vector
 * @param sunRad   Sun radius
 * @param bodyRad  Radius of body
 *
 * NOTE: this code was adapted from original SRP Shadow code. Shadow body
 * refers to the occulting body. This code implments the "Shadow Function"
 * in Montenbruck and Gil section 3.4.2
 */
//------------------------------------------------------------------------------
Real ShadowState::FindShadowState(Real *state,  Real *sbSun,
                                  Real *sunSat, Real sunRad,
                                  Real bodyRad)
{
   Real percentSun    = 1.00;   // default is full sun
   Real mag = sqrt(sbSun[0]*sbSun[0] +
                   sbSun[1]*sbSun[1] +
                   sbSun[2]*sbSun[2]);
   
   Real unitsun[3];
   unitsun[0] = sbSun[0] / mag;
   unitsun[1] = sbSun[1] / mag;
   unitsun[2] = sbSun[2] / mag;
   
   Real rdotsun = state[0]*unitsun[0] +
                  state[1]*unitsun[1] +
                  state[2]*unitsun[2];
   
   if (rdotsun > 0.0) // Sunny side of central body is always fully lit
   {
       percentSun = 1.0;
       #ifdef DEBUG_SHADOW_STATE
         MessageInterface::ShowMessage(
               "ShadowState::FindShadowState **** Body is fully lit\n");
       #endif
       return percentSun;
   }
   else
   {
      //  Compute apparent quantities like body radii and distances between bodies
      Real satToSunDist;
      Real satToBodyDist;
      Real satToSunVec[3], unitSatToSun[3], unitBodyToSat[3];
      
      satToSunVec[0] = -sunSat[0];
      satToSunVec[1] = -sunSat[1];
      satToSunVec[2] = -sunSat[2];
      
      satToSunDist = sqrt(satToSunVec[0]*satToSunVec[0]+
      satToSunVec[1]*satToSunVec[1]+satToSunVec[2]*satToSunVec[2]);
      satToBodyDist = sqrt(state[0]*state[0]+
      state[1]*state[1]+state[2]*state[2]);
      
      if (sunRad >= satToSunDist)
      {
         percentSun = 1.0;
         return percentSun;
      }
      
      if (bodyRad >= satToBodyDist)
      {
         percentSun = 0.0;
         return percentSun;
      }
      
      //Equation 3.85 in Montenbruck 3.4.2
      Real a  = asin(sunRad  / satToSunDist);
      //Equation 3.86 in Montenbruck 3.4.2
      Real b  = asin(bodyRad / satToBodyDist);
      
      unitBodyToSat[0] = state[0]/satToBodyDist;
      unitBodyToSat[1] = state[1]/satToBodyDist;
      unitBodyToSat[2] = state[2]/satToBodyDist;
      
      unitSatToSun[0] = satToSunVec[0]/satToSunDist;
      unitSatToSun[1] = satToSunVec[1]/satToSunDist;
      unitSatToSun[2] = satToSunVec[2]/satToSunDist;
      
      //Based on Equation 3.87 in Montenbruck 3.4.2 where apparentDistFromSunToBody is c
      //This equation calculates the angle between the Sun and the shadow body as seen from the SC
      //It has been adapted for efficiency and availability of parameters
      long double separationCross[3];
      for (Integer i = 0; i < 3; ++i)
      {
         const Integer j = (i + 1) % 3, k = (i + 2) % 3;
         separationCross[i] = static_cast<long double>(state[j]) * sunSat[k] -
            static_cast<long double>(state[k]) * sunSat[j];
      }
      long double crossSquared = 0, separationDot = 0;
      for (Integer i = 0; i < 3; ++i)
      {
         crossSquared += separationCross[i] * separationCross[i];
         separationDot += static_cast<long double>(state[i]) * sunSat[i];
      }
      const Real c = static_cast<Real>(std::atan2(std::sqrt(crossSquared), separationDot));
      
      if (a + b <= c)
      {
         // This is the full sun light case
         percentSun = 1.0;
         return percentSun;
      }
      else if (c < b - a)
      {
         // This is the umbra case
         percentSun = 0.0;
         return percentSun;
      }
      //Equation 3.89 in Montenbruck 3.4.2
      else if ((fabs(a - b) < c)  &&
               (     a + b  > c))
      {
          // This is the penumbra case
         Real pcbrad  = asin(bodyRad/satToBodyDist);
         Real psunrad = asin(sunRad/satToSunDist);
         PenumbraGeometry geometry(this, state, sunSat, sbSun, sunRad, bodyRad, pcbrad, psunrad);
         
         #ifdef DEBUG_SHADOW_STATE1
            MessageInterface::ShowMessage("  apparentDistFromSunToBody = %.12lf     apparentSunRadius = %.12lf  apparentBodyRadius = %.12lf\n", apparentDistFromSunToBody, apparentSunRadius, apparentBodyRadius);
         #endif
         percentSun = GetPercentSunInPenumbra(state, pcbrad, psunrad, unitSatToSun);
         return percentSun;
      }
      else
      {
      	// This is the anteumbra case
         percentSun =  1 - b*b/(a*a);
      	return percentSun;
      }
   }
   return percentSun;
   
}

//------------------------------------------------------------------------------
// protected methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Real GetPercentSunInPenumbra(Real *state,
//                              Real *sunSat, Real *force,
//                              Real pcbrad, Real psunrad)
//------------------------------------------------------------------------------
/**
 * Determines the percent sun given the input epoch, state, sun vector,
 * and radii.
 *
 * @param state    Current spacecraft state relative to its origin
 * @param unitSunToSat   Sat-to-Sun unit vector
 * @param pcbrad   Central body radius over distance
 * @param psunrad  Sun radius over distance
 *
 * NOTE: this code was adapted from original SRP Shadow code
 */
//------------------------------------------------------------------------------
Real ShadowState::GetPercentSunInPenumbra(Real *state,
                  Real pcbrad, Real psunrad, Real *unitSunToSat)
{
   const auto *geometry = PenumbraGeometry::Active();
   const bool precise = geometry && geometry->owner == this && geometry->state == state &&
      geometry->bodyAngle == pcbrad && geometry->sunAngle == psunrad;
   long double separationCross[3];
   long double crossSquared = 0, separationDot = 0, bodySquared = 0, sunSquared = 0;
   for (Integer i = 0; i < 3; ++i)
   {
      const Integer j = (i + 1) % 3, k = (i + 2) % 3;
      const long double sunI = precise ? geometry->sunVector[i] : -unitSunToSat[i];
      const long double sunJ = precise ? geometry->sunVector[j] : -unitSunToSat[j];
      const long double sunK = precise ? geometry->sunVector[k] : -unitSunToSat[k];
      separationCross[i] = static_cast<long double>(state[j]) * sunK -
         static_cast<long double>(state[k]) * sunJ;
      crossSquared += separationCross[i] * separationCross[i];
      separationDot += static_cast<long double>(state[i]) * sunI;
      bodySquared += static_cast<long double>(state[i]) * state[i];
      sunSquared += sunI * sunI;
   }
   const long double c = std::atan2(std::sqrt(crossSquared), separationDot);
   const long double a = precise ? std::asin(geometry->sunRadius / std::sqrt(sunSquared)) : psunrad;
   const long double b = precise ? std::asin(geometry->bodyRadius / std::sqrt(bodySquared)) : pcbrad;
   const long double a2 = a * a, b2 = b * b;
   const long double x = (c * c + a2 - b2) / (2 * c);
   const long double y = std::sqrt(std::max(0.0L, a2 - x * x));
   const long double first = std::acos(std::max(-1.0L, std::min(1.0L, x / a)));
   const long double second = std::acos(std::max(-1.0L, std::min(1.0L, (c - x) / b)));
   const long double area = a2 * first + b2 * second - c * y;
   return static_cast<Real>(1 - area / (std::acos(-1.0L) * a2));
}

