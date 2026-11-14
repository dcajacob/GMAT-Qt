//$Id$
//------------------------------------------------------------------------------
//                                  Barycenter
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
// Created: 2005/04/05
//
/**
 * Implementation of the Barycenter class.
 *
 */
//------------------------------------------------------------------------------

#include <vector>
#include "gmatdefs.hpp"
#include "CalculatedPoint.hpp"
#include "Barycenter.hpp"
#include <memory>
#include "EphemerisDerivative.hpp"
#include <cmath>
#include "SolarSystemException.hpp"
#include "CelestialBody.hpp"
#include "MessageInterface.hpp"
#include "GmatDefaults.hpp"
#include "SolarSystem.hpp"
#include "ColorTypes.hpp"               // for namespace GmatColor::

//#define DEBUG_BARYCENTER
//#define DEBUG_BARYCENTER_STATE
//#define DEBUG_BARYCENTER_BODIES

//---------------------------------
// static data
//---------------------------------
// none

//------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//  Barycenter(const std::string &itsName)
//------------------------------------------------------------------------------
/**
 * This method creates an object of the Barycenter class
 * (default constructor).
 *
 * @param <itsName> parameter indicating the name of the barycenter.
 */
//------------------------------------------------------------------------------
Barycenter::Barycenter(const std::string &itsName) :
   CalculatedPoint("Barycenter", itsName),
   builtInSP      (NULL)
{
   objectTypes.push_back(Gmat::BARYCENTER);
   objectTypeNames.push_back("Barycenter");
   parameterCount = BarycenterParamCount;
   
   // Set default colors
   if (itsName == GmatSolarSystemDefaults::SOLAR_SYSTEM_BARYCENTER_NAME)
      SetDefaultColors(GmatColor::TEAL, GmatColor::DARK_GRAY);
   else
      SetDefaultColors(GmatColor::GOLD, GmatColor::DARK_GRAY);
}

//------------------------------------------------------------------------------
//  Barycenter(const Barycenter &bary)
//------------------------------------------------------------------------------
/**
 * This method creates an object of the Barycenter class as a copy of the
 * specified Barycenter class (copy constructor).
 *
 * @param <bary> Barycenter object to copy.
 */
//------------------------------------------------------------------------------
Barycenter::Barycenter(const Barycenter &bary) :
   CalculatedPoint          (bary),
   builtInSP                (NULL)
{
   parameterCount  = bary.parameterCount;
}

//------------------------------------------------------------------------------
//  Barycenter& operator= (const Barycenter& bary)
//------------------------------------------------------------------------------
/**
 * Assignment operator for the Barycenter class.
 *
 * @param <bary> the Barycenter object whose data to assign to "this"
 *               calculated point.
 *
 * @return "this" Barycenter with data of input Barycenter bary.
 */
//------------------------------------------------------------------------------
Barycenter& Barycenter::operator=(const Barycenter &bary)
{
   if (&bary == this)
      return *this;

   CalculatedPoint::operator=(bary);
   parameterCount  = bary.parameterCount;
   builtInSP       = NULL; // Resolve against this value's current SolarSystem.

   return *this;
}

//------------------------------------------------------------------------------
//  ~Barycenter()
//------------------------------------------------------------------------------
/**
 * Destructor for the Barycenter class.
 */
//------------------------------------------------------------------------------
Barycenter::~Barycenter()
{
   // nothing to do here ..... hm .. hm .. hm
}

//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the Barycenter.
 *
 * @return clone of the Barycenter.
 *
 */
//------------------------------------------------------------------------------
GmatBase* Barycenter::Clone() const
{
   std::unique_ptr<Barycenter> value(new Barycenter(*this));
   value->RebindFunctionValueContext(*this);
   return value.release();
}

//---------------------------------------------------------------------------
// void Copy(const GmatBase* orig)
//---------------------------------------------------------------------------
/**
 * Sets this object to match another one.
 *
 * @param orig The original that is being copied.
 */
//---------------------------------------------------------------------------
void Barycenter::Copy(const GmatBase* orig)
{
   const auto *source = dynamic_cast<const Barycenter*>(orig);
   if (!source) throw GmatBaseException("Incompatible Barycenter value copy.\n");
   if (source == this) return;
   const auto previousContext = GetFunctionValueContext();
   // We don't want to copy instanceName
   std::string name = instanceName;
   operator=(*source);
   instanceName     = name;
   RebindFunctionValueContext(*source);
}

//---------------------------------------------------------------------------
//  const Rvector6 GetMJ2000State(const A1Mjd &atTime)
//---------------------------------------------------------------------------
/**
 * Method returning the MJ2000Eq state of the Barycenter at the time atTime.
 *
 * @param <atTime> Time for which the state is requested.
 *
 * @return state of the Barycenter at time atTime.
 */
//---------------------------------------------------------------------------
const Rvector6 Barycenter::GetMJ2000State(const A1Mjd &atTime)
{
   // if it's built-in, get the state from the SpacePoint
   if (UsesSolarSystemEphemeris())
   {
      const Rvector6 state = GetNativePoint()->GetMJ2000State(atTime);
      lastState = state;
      lastStateTime = atTime;
      lastStateTimeGT = GmatTime(atTime.Get());
      #ifdef DEBUG_BARYCENTER_STATE
         MessageInterface::ShowMessage("Computing state for Barycenter %s, whose builtInSP is %s\n",
               instanceName.c_str(), (builtInSP->GetName()).c_str());
      #endif
      return lastState;
   }
   // otherwise, sum the masses and states
   CheckBodies();
   #ifdef DEBUG_BARYCENTER
      MessageInterface::ShowMessage("Entering BaryCenter::GetMJ2000EqState at time %12.10f\n",
            atTime.Get());
   #endif
   Real     bodyMass = 0.0;
   Rvector3 bodyPos(0.0,0.0,0.0);
   Rvector3 bodyVel(0.0,0.0,0.0);

   Real     weight   = 0.0;
   Rvector3 sumMassPos(0.0,0.0,0.0);
   Rvector3 sumMassVel(0.0,0.0,0.0);
   Rvector6 bodyState;
   Real     sumMass  = GetMass();

   for (unsigned int i = 0; i < bodyList.size() ; i++)
   {
      bodyMass    = ((CelestialBody*) (bodyList.at(i)))->GetMass();
      bodyState   = (bodyList.at(i))->GetMJ2000State(atTime);
      bodyPos     = bodyState.GetR();
      bodyVel     = bodyState.GetV();
      weight      = bodyMass/sumMass;
      #ifdef DEBUG_BARYCENTER
         MessageInterface::ShowMessage("Mass (and weight) of body %s = %12.10f (%12.10f)\n",
               ((bodyList.at(i))->GetName()).c_str(), bodyMass, weight);
         MessageInterface::ShowMessage("    pos = %s\n", (bodyPos.ToString()).c_str());
         MessageInterface::ShowMessage("    vel = %s\n", (bodyVel.ToString()).c_str());
      #endif
      sumMassPos += (weight * bodyPos);
      sumMassVel += (weight * bodyVel);
   }
   #ifdef DEBUG_BARYCENTER
      MessageInterface::ShowMessage("sumMassPos = %s\n",
            (sumMassPos.ToString()).c_str());
   #endif
   lastState.Set(sumMassPos(0), sumMassPos(1), sumMassPos(2),
         sumMassVel(0), sumMassVel(1), sumMassVel(2));
   lastStateTime = atTime;
   return lastState;
}


const Rvector6 Barycenter::GetMJ2000State(const GmatTime &atTime)
{
   // if it's built-in, get the state from the SpacePoint
   if (UsesSolarSystemEphemeris())
   {
      const Rvector6 state = GetNativePoint()->GetMJ2000State(atTime);
      lastState = state;
      lastStateTimeGT = atTime;
      lastStateTime   = GmatTime(atTime).GetMjd();
#ifdef DEBUG_BARYCENTER_STATE
      MessageInterface::ShowMessage("Computing state for Barycenter %s, whose builtInSP is %s\n",
         instanceName.c_str(), (builtInSP->GetName()).c_str());
#endif
      return lastState;
   }
   // otherwise, sum the masses and states
   CheckBodies();
#ifdef DEBUG_BARYCENTER
   MessageInterface::ShowMessage("Entering BaryCenter::GetMJ2000EqState at time %12.10f\n",
      GmatTime(atTime).GetMjd());
#endif
   Real     bodyMass = 0.0;
   Rvector3 bodyPos(0.0, 0.0, 0.0);
   Rvector3 bodyVel(0.0, 0.0, 0.0);

   Real     weight = 0.0;
   Rvector3 sumMassPos(0.0, 0.0, 0.0);
   Rvector3 sumMassVel(0.0, 0.0, 0.0);
   Rvector6 bodyState;
   Real     sumMass = GetMass();

   for (unsigned int i = 0; i < bodyList.size(); i++)
   {
      bodyMass = ((CelestialBody*)(bodyList.at(i)))->GetMass();
      bodyState = (bodyList.at(i))->GetMJ2000State(atTime);
      bodyPos = bodyState.GetR();
      bodyVel = bodyState.GetV();
      weight = bodyMass / sumMass;
#ifdef DEBUG_BARYCENTER
      MessageInterface::ShowMessage("Mass (and weight) of body %s = %12.10f (%12.10f)\n",
         ((bodyList.at(i))->GetName()).c_str(), bodyMass, weight);
      MessageInterface::ShowMessage("    pos = %s\n", (bodyPos.ToString()).c_str());
      MessageInterface::ShowMessage("    vel = %s\n", (bodyVel.ToString()).c_str());
#endif
      sumMassPos += (weight * bodyPos);
      sumMassVel += (weight * bodyVel);
   }
#ifdef DEBUG_BARYCENTER
   MessageInterface::ShowMessage("sumMassPos = %s\n",
      (sumMassPos.ToString()).c_str());
#endif
   lastState.Set(sumMassPos(0), sumMassPos(1), sumMassPos(2),
      sumMassVel(0), sumMassVel(1), sumMassVel(2));
   
   lastStateTimeGT = atTime;
   lastStateTime   = GmatTime(atTime).GetMjd();

   return lastState;
}


//---------------------------------------------------------------------------
//  const Rvector3 GetMJ2000Position(const A1Mjd &atTime)
//---------------------------------------------------------------------------
/**
 * Method returning the MJ2000Eq position of the Barycenter at the time atTime.
 *
 * @param <atTime> Time for which the position is requested.
 *
 * @return position of the Barycenter at time atTime.
 */
//---------------------------------------------------------------------------
const Rvector3 Barycenter::GetMJ2000Position(const A1Mjd &atTime)
{
   Rvector6 tmp = GetMJ2000State(atTime);
   return (tmp.GetR());
}


const Rvector3 Barycenter::GetMJ2000Position(const GmatTime &atTime)
{
   Rvector6 tmp = GetMJ2000State(atTime);
   return (tmp.GetR());
}


//---------------------------------------------------------------------------
//  const Rvector3 GetMJ2000Velocity(const A1Mjd &atTime)
//---------------------------------------------------------------------------
/**
 * Method returning the MJ2000Eq velocity of the Barycenter at the time atTime.
 *
 * @param <atTime> Time for which the velocity is requested.
 *
 * @return velocity of the Barycenter at time atTime.
 */
//---------------------------------------------------------------------------
const Rvector3 Barycenter::GetMJ2000Velocity(const A1Mjd &atTime)
{
   Rvector6 tmp = GetMJ2000State(atTime);
   return (tmp.GetV());
}


const Rvector3 Barycenter::GetMJ2000Velocity(const GmatTime &atTime)
{
   Rvector6 tmp = GetMJ2000State(atTime);
   return (tmp.GetV());
}


// Native MJ2000 acceleration, in km/s^2.
const Rvector3 Barycenter::GetMJ2000Acceleration(const A1Mjd &atTime)
{
   return GetMJ2000Acceleration(GmatTime(atTime.Get()));
}

const Rvector3 Barycenter::GetMJ2000Acceleration(const GmatTime &atTime)
{
   GmatEphemerisDetail::ScopedDerivativeQuery uncached;
   Rvector3 acceleration(0.0, 0.0, 0.0);
   if (UsesSolarSystemEphemeris())
   {
      SpacePoint *provider = GetNativePoint();
      if (!provider->IsOfType(Gmat::CELESTIAL_BODY))
         throw SolarSystemException("Barycenter acceleration requires a native built-in body provider for " + instanceName);
      acceleration = provider->GetMJ2000Acceleration(atTime);
   }
   else
   {
      CheckBodies();
      if (bodyList.size() != bodyNames.size())
         throw SolarSystemException("Barycenter acceleration requires all body references to be resolved for " + instanceName);
      const Real totalMass = GetMass();
      if (!std::isfinite(totalMass) || totalMass <= 0.0)
         throw SolarSystemException("Barycenter acceleration requires a finite positive total mass for " + instanceName);
      // These are the same constant mass weights used by GetMJ2000State.
      for (auto *point : bodyList)
      {
         auto *body = static_cast<CelestialBody *>(point); // CheckBodies validates the type.
         const Real mass = body->GetMass();
         if (!std::isfinite(mass) || mass < 0.0)
            throw SolarSystemException("Barycenter acceleration requires finite nonnegative body masses for " + instanceName);
         if (body->GetJ2000BodyName() != GetJ2000BodyName())
            throw SolarSystemException("Barycenter acceleration requires a common J2000 reference body for " + instanceName);
         acceleration += (mass / totalMass) * body->GetMJ2000Acceleration(atTime);
      }
   }
   for (Integer i = 0; i < 3; ++i)
      if (!std::isfinite(acceleration[i]))
         throw SolarSystemException("Nonfinite MJ2000 acceleration for " + instanceName);
   GetMJ2000State(atTime); // Leave point and body epoch caches at the query time.
   return acceleration;
}

//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//                   const std::string &name, const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets the reference object.
 *
 * @param <obj>   reference object pointer.
 * @param <type>  type of the reference object.
 * @param <name>  name of the reference object.
 * @param <index> Index into the object array.
 *
 * @return success of the operation.
 */
//------------------------------------------------------------------------------
bool Barycenter::SetRefObject(GmatBase *obj,
                              const UnsignedInt type,
                              const std::string &name)
{
   if (!obj) return false;
   if (UsesSolarSystemEphemeris())
      return SpacePoint::SetRefObject(obj, type, name);
   if ((obj->IsOfType(Gmat::SPACE_POINT)) && (!obj->IsOfType("CelestialBody")))
   {
      std::string errmsg = "The value of \"";
      errmsg += name + "\" for field \"BodyNames\" on CalculatedPoint \"";;
      errmsg += instanceName + "\" is not an allowed value.\n";
      errmsg += "The allowed values are: [CelestialBody or Barycenter (except SSB)].\n";
      throw SolarSystemException(errmsg);
   }

   return CalculatedPoint::SetRefObject(obj, type, name);
}


//---------------------------------------------------------------------------
//  Real GetMass()
//---------------------------------------------------------------------------
/**
 * Method returning the total mass of the celestial bodies included in
 * this Barycenter.
 *
 * @return total mass of the celestial bodies included in this Barycenter.
 */
//---------------------------------------------------------------------------
Real Barycenter::GetMass() 
{
   // if it's built-in, get the mass from the SpacePoint
   if (UsesSolarSystemEphemeris())
   {
      auto *provider = dynamic_cast<CelestialBody *>(GetNativePoint());
      if (!provider)
         throw SolarSystemException("Barycenter mass requires a native body provider for " + instanceName);
      return provider->GetMass();
   }
   // otherwise, sum the masses of the bodies
   Real     sumMass  = 0.0;
   for (unsigned int i = 0; i < bodyList.size() ; i++)
   {
      sumMass    += ((CelestialBody*) (bodyList.at(i)))->GetMass();
   }
   return sumMass;
}


//---------------------------------------------------------------------------
//  bool Initialize()
//---------------------------------------------------------------------------
/**
 * Method that initializes this Barycenter.
 *
 * @return success flag for initializing
 */
//---------------------------------------------------------------------------
bool Barycenter::Initialize()
{
   #ifdef DEBUG_BARYCENTER_BODIES
      MessageInterface::ShowMessage("Entering Barycenter::Initialize\n   bodyNames:\n");
      for (unsigned int ii = 0; ii < bodyNames.size(); ii++)
         MessageInterface::ShowMessage("   %d    %s\n", ii, (bodyNames.at(ii)).c_str());
      MessageInterface::ShowMessage("and defaultBodies:\n");
      for (unsigned int ii = 0; ii < defaultBodies.size(); ii++)
         MessageInterface::ShowMessage("   %d    %s\n", ii, (defaultBodies.at(ii)).c_str());
   #endif
   if (UsesSolarSystemEphemeris())
   {
      GetNativePoint();
   }
   else
   {
      if ((bodyNames.empty()) && !(defaultBodies.empty()))
      {
         for (unsigned int ii = 0; ii < defaultBodies.size(); ii++)
            bodyNames.push_back(defaultBodies.at(ii));
      }
      if (bodyNames.empty())
      {
         std::string errmsg = "No celestial body specified for Barycenter ";
         errmsg += instanceName + "\n";
         throw SolarSystemException(errmsg);
      }
      #ifdef DEBUG_BARYCENTER_BODIES
         MessageInterface::ShowMessage("at end of Barycenter::Initialize\n   bodyNames:\n");
         for (unsigned int ii = 0; ii < bodyNames.size(); ii++)
            MessageInterface::ShowMessage("   %d    %s\n", ii, (bodyNames.at(ii)).c_str());
      #endif
   }
   return CalculatedPoint::Initialize();
}

//---------------------------------------------------------------------------
//  StringArray GetBuiltInNames()
//---------------------------------------------------------------------------
/**
 * Returns the name(s) of the built-in space point.
 *
 * @return  name of the space point for the built-in barycenter
 */
//---------------------------------------------------------------------------
StringArray Barycenter::GetBuiltInNames()
{
   StringArray spNames;
   if (UsesSolarSystemEphemeris())
      spNames.push_back("SolarSystemBarycenter");
   return spNames;
}

bool Barycenter::UsesSolarSystemEphemeris() const
{
   return isBuiltIn || builtInType == "SSB" ||
      builtInType == GmatSolarSystemDefaults::SOLAR_SYSTEM_BARYCENTER_NAME;
}

SpacePoint* Barycenter::GetNativePoint()
{
   if (builtInType != "SSB" &&
         builtInType != GmatSolarSystemDefaults::SOLAR_SYSTEM_BARYCENTER_NAME)
      throw SolarSystemException("Unknown native barycenter source for " + instanceName);
   if (!theSolarSystem)
      throw SolarSystemException("No SolarSystem is bound to barycenter " + instanceName);
   SpacePoint *provider = theSolarSystem->GetSpecialPoint(
      GmatSolarSystemDefaults::SOLAR_SYSTEM_BARYCENTER_NAME);
   if (!provider)
      throw SolarSystemException("No native SolarSystemBarycenter provider for " + instanceName);
   // Re-resolve on every query. Copies and provider-context changes must not
   // use a pointer borrowed from a different SolarSystem instance.
   builtInSP = provider;
   return provider;
}

void Barycenter::MakeUserDefinedCopy()
{
   const bool native = UsesSolarSystemEphemeris();
   SetIsBuiltIn(false, native ? "SSB" : "");
   builtInSP = NULL;
   isInitialized = false;
}

std::string Barycenter::GetParameterText(const Integer id) const
{
   if (id == EPHEMERIS_SOURCE) return "EphemerisSource";
   return CalculatedPoint::GetParameterText(id);
}

Integer Barycenter::GetParameterID(const std::string &str) const
{
   if (str == "EphemerisSource") return EPHEMERIS_SOURCE;
   return CalculatedPoint::GetParameterID(str);
}

Gmat::ParameterType Barycenter::GetParameterType(const Integer id) const
{
   if (id == EPHEMERIS_SOURCE) return Gmat::ENUMERATION_TYPE;
   return CalculatedPoint::GetParameterType(id);
}

bool Barycenter::IsParameterReadOnly(const Integer id) const
{
   if (id == EPHEMERIS_SOURCE) return isBuiltIn;
   if (id == BODY_NAMES && UsesSolarSystemEphemeris()) return true;
   return CalculatedPoint::IsParameterReadOnly(id);
}

bool Barycenter::IsParameterCommandModeSettable(const Integer id) const
{
   if (id == EPHEMERIS_SOURCE) return false;
   return CalculatedPoint::IsParameterCommandModeSettable(id);
}

std::string Barycenter::GetStringParameter(const Integer id) const
{
   if (id == EPHEMERIS_SOURCE)
      return UsesSolarSystemEphemeris() ? "SolarSystemBarycenter" : "MassWeighted";
   return CalculatedPoint::GetStringParameter(id);
}

std::string Barycenter::GetStringParameter(const std::string &label) const
{
   return GetStringParameter(GetParameterID(label));
}

bool Barycenter::SetStringParameter(const Integer id, const std::string &value)
{
   if (id == EPHEMERIS_SOURCE)
   {
      if (value != "MassWeighted" && value != "SolarSystemBarycenter")
         throw SolarSystemException("EphemerisSource must be MassWeighted or SolarSystemBarycenter for " + instanceName);
      if (isBuiltIn && value != "SolarSystemBarycenter")
         throw SolarSystemException("Cannot change the built-in SolarSystemBarycenter calculation source.");
      if (value == GetStringParameter(id)) return true;
      std::string nextType = value == "SolarSystemBarycenter" ? "SSB" : "";
      StringArray defaults = defaultBodies;
      if (value == "MassWeighted" && bodyNames.empty() && defaults.empty())
      {
         defaults.push_back(GmatSolarSystemDefaults::EARTH_NAME);
         defaults.push_back(GmatSolarSystemDefaults::MOON_NAME);
      }
      builtInType.swap(nextType);
      defaultBodies.swap(defaults);
      builtInSP = NULL;
      isInitialized = false;
      if (UsesSolarSystemEphemeris())
      {
         bodyNames.clear();
         bodyList.clear();
         numberOfBodies = 0;
      }
      return true;
   }
   // Preserve the inherited immutable-membership diagnostic for built-in points.
   if (id == BODY_NAMES && UsesSolarSystemEphemeris() && !isBuiltIn)
      throw SolarSystemException("BodyNames is not used with the native SolarSystemBarycenter ephemeris source.");
   return CalculatedPoint::SetStringParameter(id, value);
}

bool Barycenter::SetStringParameter(const Integer id, const std::string &value,
                                  const Integer index)
{
   if (id == BODY_NAMES && UsesSolarSystemEphemeris() && !isBuiltIn)
      throw SolarSystemException("BodyNames is not used with the native SolarSystemBarycenter ephemeris source.");
   return CalculatedPoint::SetStringParameter(id, value, index);
}

const StringArray& Barycenter::GetStringArrayParameter(const Integer id) const
{
   if (id == BODY_NAMES && UsesSolarSystemEphemeris())
   {
      static const StringArray none;
      return none;
   }
   return CalculatedPoint::GetStringArrayParameter(id);
}

const StringArray& Barycenter::GetPropertyEnumStrings(const Integer id) const
{
   if (id == EPHEMERIS_SOURCE)
   {
      static const StringArray sources{"MassWeighted", "SolarSystemBarycenter"};
      return sources;
   }
   return CalculatedPoint::GetPropertyEnumStrings(id);
}

const StringArray& Barycenter::GetRefObjectNameArray(const UnsignedInt type)
{
   if (UsesSolarSystemEphemeris() &&
         (type == Gmat::UNKNOWN_OBJECT || type == Gmat::SPACE_POINT))
   {
      static const StringArray none;
      return none;
   }
   return CalculatedPoint::GetRefObjectNameArray(type);
}

bool Barycenter::TakeRequiredAction(const Integer id)
{
   if (id == BODY_NAMES && UsesSolarSystemEphemeris())
      throw SolarSystemException("Cannot replace BodyNames for a native ephemeris barycenter.");
   return CalculatedPoint::TakeRequiredAction(id);
}

//------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  void CheckBodies()
//------------------------------------------------------------------------------
/**
 * Method for the Barycenter class that checks to make sure the bodyList has
 * been defined appropriately (i.e. all CelestialBody objects).
 *
 */
//------------------------------------------------------------------------------
void Barycenter::CheckBodies()
{
   if ((bodyNames.empty()) && !(defaultBodies.empty()))
   {
      for (unsigned int ii = 0; ii < defaultBodies.size(); ii++)
         bodyNames.push_back(defaultBodies.at(ii));
   }
   if (bodyNames.empty())
      throw SolarSystemException("Attempting to use Barycenter with no bodies set ...\n");

   for (unsigned int i = 0; i < bodyList.size() ; i++)
      if ((bodyList.at(i))->GetType() != Gmat::CELESTIAL_BODY)
         throw SolarSystemException(
         "Barycenter defined incorrectly with non-Celestial Body components");
}


